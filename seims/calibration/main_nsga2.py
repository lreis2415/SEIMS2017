# coding:utf-8
"""Calibration by NSGA-II algorithm.

    @author   : Liangjun Zhu

    @changelog:
    - 18-01-22  - lj - initial implementation.
    - 18-02-09  - lj - compatible with Python3.
    - 18-07-10  - lj - Support MPI version of SEIMS.
    - 18-08-26  - lj - Gather the execute time of all model runs. Plot pareto graphs.
    - 18-08-29  - jz,lj,sf - Add Nutrient calibration step.
    - 18-10-22  - lj - Make the customizations of multi-objectives flexible.
"""
from __future__ import absolute_import, division, unicode_literals

import array
import os
import random
import time
import sys
from io import open

if os.path.abspath(os.path.join(sys.path[0], '..')) not in sys.path:
    sys.path.insert(0, os.path.abspath(os.path.join(sys.path[0], '..')))

from typing import Dict
import numpy
from deap import base
from deap import creator
from deap import tools
from deap.benchmarks.tools import hypervolume
from copy import deepcopy
from pygeoc.utils import UtilClass

from utility.scoop_func import scoop_log
from scenario_analysis.userdef import initIterateWithCfg, initRepeatWithCfg
from scenario_analysis.visualization import plot_pareto_front_single, plot_hypervolume_single
from calibration.config import CaliConfig, get_optimization_config
from run_seims import MainSEIMS

from calibration.calibrate import Calibration, initialize_calibrations, calibration_objectives
from calibration.calibrate import TimeseriesData, ObsSimData
from calibration.userdef import write_param_values_to_mongodb, output_population_details

# Definitions, assignments, operations, etc. that will be executed by each worker
#    when paralleled by SCOOP.
# Thus, DEAP related operations (initialize, register, etc.) are better defined here.

# All accepted objective function names from `postprocess::utility::calculate_statistics`
accepted_objnames = ['NSE', 'RSR', 'PBIAS', 'R-square', 'RMSE', 'lnNSE', 'NSE1', 'NSE3']

# step can be one of 'Q', 'SED', 'QSED', 'NUTRIENT', or 'CUSTOMIZE'.
step = 'Q'
filter_ind = False  # Filter for valid population for the next generation
# Definitions of Multiobjectives:
multiobj = dict()
if step == 'Q':
    # Step 1: Calibrate streamflow, max. NSE, min. RSR, and min. |PBIAS| (percent)
    multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
elif step == 'SED':
    # Step 2: Calibration sediment, max. NSE-SED, min. RSR-SED, min., and |PBIAS|-SED
    multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100], ['PBIAS', -1., 500.]])
elif step == 'QSED':
    # Step 3: Calibration streamflow and sediment
    multiobj.setdefault('Q', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
    multiobj.setdefault('SED', [['NSE', 1., -100, '>0'], ['RSR', -1., 100, '<1']])
elif step == 'NUTRIENT':
    # Step 4: Calibration NSE-TN, NSE-TP, NSE-Q, NSE-SED
    multiobj.setdefault('CH_TN', [['NSE', 1., -100]])
    multiobj.setdefault('CH_TP', [['NSE', 1., -100]])
    multiobj.setdefault('Q', [['NSE', 1., -100]])
    multiobj.setdefault('SED', [['NSE', 1., -100]])
else:
    # Customize your own multiobjective here, such as:
    multiobj.setdefault('Q', [['NSE', 3., 0., '>0.'],
                              ['RSR', -1., 2., '<2.'],
                              ['PBIAS', -1., 50., '<50.']])
    multiobj.setdefault('SED', [['NSE', 3., 0., '>0.'],
                                ['RSR', -1., 2., '<2.'],
                                ['PBIAS', -1., 100., '<100.']])

# Check object variables
if not multiobj:
    print('Multiobjective MUST not be Empty!')
    exit(1)
for k, v in list(multiobj.items()):
    for item in v:
        if len(item) < 3:
            print('Each item of objective MUST have three elements, '
                  'i.e., object name, weight, worse value for Hypervolum calculation.')
            exit(1)
        if item[0] not in accepted_objnames:
            print('Object name % is unsupported! '
                  'Please input one of %s!' % (item[0], ','.join(accepted_objnames)))
            exit(1)
# Get parameters from `multiobj`
object_vars = list(multiobj.keys())
object_names = dict({k: list(l[0] for l in v) for k, v in list(multiobj.items())})
multi_weight = tuple(l[1] for v in multiobj.values() for l in v)
worse_objects = list(l[2] for v in multiobj.values() for l in v)
conditions = list(l[3] if (len(l) > 3) else None for v in multiobj.values() for l in v)

creator.create('FitnessMulti', base.Fitness, weights=multi_weight)
# The FitnessMulti class equals to (as an example):
# class FitnessMulti(base.Fitness):
#     weights = (2., -1., -1.)
# NOTE that to maintain the compatibility with Python2 and Python3,
#      the com typecode=str('d') MUST NOT changed to typecode='d', since
#      the latter will raise TypeError that 'must be char, not unicode'!
creator.create('Individual', array.array, typecode=str('d'), fitness=creator.FitnessMulti,
               gen=-1, id=-1,
               obs=TimeseriesData, sim=TimeseriesData,
               cali=ObsSimData, vali=ObsSimData,
               io_time=0., comp_time=0., simu_time=0., runtime=0.)
# The Individual class equals to:
# class Individual(array.array):
#     gen = -1  # Generation No.
#     id = -1   # Calibration index of current generation
#     def __init__(self):
#         self.fitness = FitnessMulti()

# Register NSGA-II related operations
toolbox = base.Toolbox()
toolbox.register('gene_values', initialize_calibrations)
toolbox.register('individual', initIterateWithCfg, creator.Individual, toolbox.gene_values)
toolbox.register('population', initRepeatWithCfg, list, toolbox.individual)
toolbox.register('evaluate', calibration_objectives)

# mate and mutate
toolbox.register('mate', tools.cxSimulatedBinaryBounded)
toolbox.register('mutate', tools.mutPolynomialBounded)
toolbox.register('select', tools.selNSGA2)


def main(cfg):
    """Main workflow of NSGA-II based Scenario analysis."""
    random.seed()
    scoop_log('Population: %d, Generation: %d' % (cfg.opt.npop, cfg.opt.ngens))

    # Initial timespan variables
    stime = time.time()
    plot_time = 0.
    allmodels_exect = list()  # execute time of all model runs

    # create reference point for hypervolume
    ref_pt = numpy.array(worse_objects) * multi_weight * -1

    stats = tools.Statistics(lambda sind: sind.fitness.values)
    stats.register('min', numpy.min, axis=0)
    stats.register('max', numpy.max, axis=0)
    stats.register('avg', numpy.mean, axis=0)
    stats.register('std', numpy.std, axis=0)
    logbook = tools.Logbook()
    logbook.header = 'gen', 'evals', 'min', 'max', 'avg', 'std'

    # read observation data from MongoDB
    cali_obj = Calibration(cfg)

    # Read observation data just once
    model_cfg_dict = cali_obj.model.ConfigDict
    model_obj = MainSEIMS(args_dict=model_cfg_dict)

    model_obj.SetMongoClient()
    obs_vars, obs_data_dict = model_obj.ReadOutletObservations(object_vars)
    model_obj.UnsetMongoClient()

    # Initialize population
    param_values = cali_obj.initialize(cfg.opt.npop)
    pop = list()
    for i in range(cfg.opt.npop):
        ind = creator.Individual(param_values[i])
        ind.gen = 0
        ind.id = i
        ind.obs.vars = obs_vars[:]
        ind.obs.data = deepcopy(obs_data_dict)
        pop.append(ind)
    param_values = numpy.array(param_values)

    # Write calibrated values to MongoDB
    # TODO, extract this function, which is same with `Sensitivity::write_param_values_to_mongodb`.
    write_param_values_to_mongodb(cfg.model.host, cfg.model.port, cfg.model.db_name, cali_obj.ParamDefs, param_values)
    # get the low and up bound of calibrated parameters
    bounds = numpy.array(cali_obj.ParamDefs['bounds'])
    low = bounds[:, 0]
    up = bounds[:, 1]
    low = low.tolist()
    up = up.tolist()
    pop_select_num = int(cfg.opt.npop * cfg.opt.rsel)
    init_time = time.time() - stime

    def check_validation(fitvalues):
        """Check the validation of the fitness values of an individual."""
        flag = True
        for condidx, condstr in enumerate(conditions):
            if condstr is None:
                continue
            if not eval('%f%s' % (fitvalues[condidx], condstr)):
                flag = False
        return flag

    def evaluate_parallel(invalid_pops):
        """Evaluate model by SCOOP or map, and set fitness of individuals
         according to calibration step."""
        popnum = len(invalid_pops)
        labels = list()
        try:  # parallel on multi-processors or clusters using SCOOP
            from scoop import futures
            invalid_pops = list(futures.map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops))
        except ImportError or ImportWarning:  # Python build-in map (serial)
            invalid_pops = list(map(toolbox.evaluate, [cali_obj] * popnum, invalid_pops))
        for tmpind in invalid_pops:
            labels = list()  # TODO, find an elegant way to get labels.
            tmpfitnessv = list()
            for k, v in list(multiobj.items()):
                tmpvalues, tmplabel = tmpind.cali.efficiency_values(k, object_names[k])
                tmpfitnessv += tmpvalues[:]
                labels += tmplabel[:]
            tmpind.fitness.values = tuple(tmpfitnessv)

        # Filter for a valid solution
        if filter_ind:
            invalid_pops = [tmpind for tmpind in invalid_pops
                            if check_validation(tmpind.fitness.values)]
            if len(invalid_pops) < 2:
                print('The initial population should be greater or equal than 2. '
                      'Please check the parameters ranges or change the sampling strategy!')
                exit(2)
        return invalid_pops, labels  # Currently, `invalid_pops` contains evaluated individuals

    # Record the count and execute timespan of model runs during the optimization
    modelruns_count = {0: len(pop)}
    modelruns_time = {0: 0.}  # Total time counted according to evaluate_parallel()
    modelruns_time_sum = {0: 0.}  # Summarize time of every model runs according to pop

    # Generation 0 before optimization
    stime = time.time()
    pop, plotlables = evaluate_parallel(pop)
    modelruns_time[0] = time.time() - stime
    for ind in pop:
        allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
        modelruns_time_sum[0] += ind.runtime

    # currently, len(pop) may less than pop_select_num
    pop = toolbox.select(pop, pop_select_num)
    # Output simulated data to json or pickle files for future use.
    output_population_details(pop, cfg.opt.simdata_dir, 0, plot_cfg=cali_obj.cfg.plot_cfg)

    record = stats.compile(pop)
    logbook.record(gen=0, evals=len(pop), **record)
    scoop_log(logbook.stream)

    # Begin the generational process
    output_str = '### Generation number: %d, Population size: %d ###\n' % (cfg.opt.ngens,
                                                                           cfg.opt.npop)
    scoop_log(output_str)
    UtilClass.writelog(cfg.opt.logfile, output_str, mode='replace')

    modelsel_count = {0: len(pop)}  # type: Dict[int, int] # newly added Pareto fronts

    for gen in range(1, cfg.opt.ngens + 1):
        output_str = '###### Generation: %d ######\n' % gen
        scoop_log(output_str)

        offspring = [toolbox.clone(ind) for ind in pop]
        # method1: use crowding distance (normalized as 0~1) as eta
        # tools.emo.assignCrowdingDist(offspring)
        # method2: use the index of individual at the sorted offspring list as eta
        if len(offspring) >= 2:  # when offspring size greater than 2, mate can be done
            for i, ind1, ind2 in zip(range(len(offspring) // 2), offspring[::2], offspring[1::2]):
                if random.random() > cfg.opt.rcross:
                    continue
                eta = i
                toolbox.mate(ind1, ind2, eta, low, up)
                toolbox.mutate(ind1, eta, low, up, cfg.opt.rmut)
                toolbox.mutate(ind2, eta, low, up, cfg.opt.rmut)
                del ind1.fitness.values, ind2.fitness.values
        else:
            toolbox.mutate(offspring[0], 1., low, up, cfg.opt.rmut)
            del offspring[0].fitness.values

        # Evaluate the individuals with an invalid fitness
        invalid_inds = [ind for ind in offspring if not ind.fitness.valid]
        valid_inds = [ind for ind in offspring if ind.fitness.valid]
        if len(invalid_inds) == 0:  # No need to continue
            scoop_log('Note: No invalid individuals available, the NSGA2 will be terminated!')
            break

        # Write new calibrated parameters to MongoDB
        param_values = list()
        for idx, ind in enumerate(invalid_inds):
            ind.gen = gen
            ind.id = idx
            param_values.append(ind[:])
        param_values = numpy.array(param_values)
        write_param_values_to_mongodb(cfg.model.host, cfg.model.port, cfg.model.db_name,
                                      cali_obj.ParamDefs, param_values)
        # Count the model runs, and execute models
        invalid_ind_size = len(invalid_inds)
        modelruns_count.setdefault(gen, invalid_ind_size)
        stime = time.time()
        invalid_inds, plotlables = evaluate_parallel(invalid_inds)
        curtimespan = time.time() - stime
        modelruns_time.setdefault(gen, curtimespan)
        modelruns_time_sum.setdefault(gen, 0.)
        for ind in invalid_inds:
            allmodels_exect.append([ind.io_time, ind.comp_time, ind.simu_time, ind.runtime])
            modelruns_time_sum[gen] += ind.runtime

        # Select the next generation population
        # Previous version may result in duplications of the same scenario in one Pareto front,
        #   thus, I decided to check and remove the duplications first.
        # pop = toolbox.select(pop + valid_inds + invalid_inds, pop_select_num)
        tmppop = pop + valid_inds + invalid_inds
        pop = list()
        unique_sces = dict()
        for tmpind in tmppop:
            if tmpind.gen in unique_sces and tmpind.id in unique_sces[tmpind.gen]:
                continue
            if tmpind.gen not in unique_sces:
                unique_sces.setdefault(tmpind.gen, [tmpind.id])
            elif tmpind.id not in unique_sces[tmpind.gen]:
                unique_sces[tmpind.gen].append(tmpind.id)
            pop.append(tmpind)
        pop = toolbox.select(pop, pop_select_num)

        output_population_details(pop, cfg.opt.simdata_dir, gen, plot_cfg=cali_obj.cfg.plot_cfg)
        hyper_str = 'Gen: %d, New model runs: %d, ' \
                    'Execute timespan: %.4f, Sum of model run timespan: %.4f, ' \
                    'Hypervolume: %.4f\n' % (gen, invalid_ind_size,
                                             curtimespan, modelruns_time_sum[gen],
                                             hypervolume(pop, ref_pt))
        scoop_log(hyper_str)
        UtilClass.writelog(cfg.opt.hypervlog, hyper_str, mode='append')

        record = stats.compile(pop)
        logbook.record(gen=gen, evals=len(invalid_inds), **record)
        scoop_log(logbook.stream)

        # Count the newly generated near Pareto fronts
        new_count = 0
        for ind in pop:
            if ind.gen == gen:
                new_count += 1
        modelsel_count.setdefault(gen, new_count)

        # Plot 2D near optimal pareto front graphs,
        #   i.e., (NSE, RSR), (NSE, PBIAS), and (RSR,PBIAS)
        # And 3D near optimal pareto front graphs, i.e., (NSE, RSR, PBIAS)
        stime = time.time()
        front = numpy.array([ind.fitness.values for ind in pop])
        title = (u'近似最优Pareto解集' if cali_obj.cfg.plot_cfg.plot_cn else
                 'Near Pareto optimal solutions')

        plot_pareto_front_single(front, plotlables, cfg.opt.out_dir,
                                 gen, title, plot_cfg=cali_obj.cfg.plot_cfg)
        plot_time += time.time() - stime

        # save in file
        # Header information
        output_str += 'generation\tcalibrationID\t'
        for kk, vv in list(object_names.items()):
            output_str += pop[0].cali.output_header(kk, vv, 'Cali')
        if cali_obj.cfg.calc_validation:
            for kkk, vvv in list(object_names.items()):
                output_str += pop[0].vali.output_header(kkk, vvv, 'Vali')

        output_str += 'gene_values\n'
        for ind in pop:
            output_str += '%d\t%d\t' % (ind.gen, ind.id)
            for kk, vv in list(object_names.items()):
                output_str += ind.cali.output_efficiency(kk, vv)
            if cali_obj.cfg.calc_validation:
                for kkk, vvv in list(object_names.items()):
                    output_str += ind.vali.output_efficiency(kkk, vvv)
            output_str += str(ind)
            output_str += '\n'
        UtilClass.writelog(cfg.opt.logfile, output_str, mode='append')

        # TODO: Figure out if we should terminate the evolution

    # Plot hypervolume and newly executed model count
    plot_hypervolume_single(cfg.opt.hypervlog, cfg.opt.out_dir, plot_cfg=cali_obj.cfg.plot_cfg)

    # Save newly added Pareto fronts of each generations
    new_fronts_count = numpy.array(list(modelsel_count.items()))
    numpy.savetxt('%s/new_pareto_fronts_count.txt' % cfg.opt.out_dir,
                  new_fronts_count, delimiter=str(','), fmt=str('%d'))

    # Save and print timespan information
    allmodels_exect = numpy.array(allmodels_exect)
    numpy.savetxt('%s/exec_time_allmodelruns.txt' % cfg.opt.out_dir,
                  allmodels_exect, delimiter=str(' '), fmt=str('%.4f'))
    scoop_log('Running time of all SEIMS models:\n'
              '\tIO\tCOMP\tSIMU\tRUNTIME\n'
              'MAX\t%s\n'
              'MIN\t%s\n'
              'AVG\t%s\n'
              'SUM\t%s\n' % ('\t'.join('%.3f' % t for t in allmodels_exect.max(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.min(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.mean(0)),
                             '\t'.join('%.3f' % t for t in allmodels_exect.sum(0))))

    exec_time = 0.
    for genid, tmptime in list(modelruns_time.items()):
        exec_time += tmptime
    exec_time_sum = 0.
    for genid, tmptime in list(modelruns_time_sum.items()):
        exec_time_sum += tmptime
    allcount = 0
    for genid, tmpcount in list(modelruns_count.items()):
        allcount += tmpcount

    scoop_log('Initialization timespan: %.4f\n'
              'Model execution timespan: %.4f\n'
              'Sum of model runs timespan: %.4f\n'
              'Plot Pareto graphs timespan: %.4f' % (init_time, exec_time,
                                                     exec_time_sum, plot_time))

    return pop, logbook


if __name__ == "__main__":
    cf, method = get_optimization_config()
    cali_cfg = CaliConfig(cf, method=method)

    scoop_log('### START TO CALIBRATION OPTIMIZING ###')
    startT = time.time()

    fpop, fstats = main(cali_cfg)

    fpop.sort(key=lambda x: x.fitness.values)
    scoop_log(fstats)
    with open(cali_cfg.opt.logbookfile, 'w', encoding='utf-8') as f:
        # In case of 'TypeError: write() argument 1 must be unicode, not str' in Python2.7
        #   when using unicode_literals, please use '%s' to concatenate string!
        f.write('%s' % fstats.__str__())
    endT = time.time()
    scoop_log('### END OF CALIBRATION OPTIMIZING ###')
    scoop_log('Running time: %.2fs' % (endT - startT))
