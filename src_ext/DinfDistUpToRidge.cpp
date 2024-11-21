/*  DinfDistUpToRidge function to compute distance to ridge in DEM
    based on D-infinity flow direction model, ridge is
	assigned by user.

  Liangjun Zhu
  Lreis, CAS
  Apr 2, 2015

    Changelog: 17-08-09  lj - There are two circumstances will end the search for upstream ridge
                              for a cell:
                                1) trace upstream and reach a ridge;
                                2) trace upstream and the terminal cell is not flagged as a ridge,
                                   then find the nearest ridge around this cell. In this case, the
                                   distance (h, v, or s) are summed up by two distances.

*/
#include <mpi.h>
#include <math.h>
#include <queue>
#include "commonLib.h"
#include "createpart.h"
#include "tiffIO.h"
#include "DinfDistUpToRidge.h"
#include "initneighbor.h"

// using namespace std; // Avoid to using the entire namespace of std. Comment by Liangjun, 01/23/19


float dist[9];

//Calling function
int dinfdistup(char *angfile, char *felfile, char *slpfile, char *rdgfile, char *wfile, char *rtrfile,
               int statmethod, int typemethod, int userdg, int usew, int concheck, float thresh)
{
    int er = -1;
    switch (typemethod)
    {
        case 0:
            er = hdisttoridgegrd(angfile, rdgfile, wfile, rtrfile, statmethod,
                                 concheck, thresh, userdg, usew);
            break;
        case 1:
            er = vrisetoridgegrd(angfile, felfile, rdgfile, rtrfile,
                                 statmethod, concheck, thresh, userdg);
            break;
        case 2:
            er = pdisttoridgegrd(angfile, felfile, rdgfile, wfile, rtrfile,
                                 statmethod, userdg, usew, concheck, thresh);
            break;
        case 3:
            er = sdisttoridgegrd(angfile, felfile, rdgfile, wfile, rtrfile,
                                 statmethod, userdg, usew, concheck, thresh);
            break;
    }
    return (er);
}

//*****************************//
//Horizontal distance to ridge //
//*****************************//
int hdisttoridgegrd(char *angfile, char *rdgfile, char *wfile, char *rtrfile, int statmethod,
                    int concheck, float thresh, int userdg, int usew)
{
    MPI_Init(NULL, NULL);
    {

        //Only used for timing
        int rank, size;
        MPI_Comm_rank(MCW, &rank);
        MPI_Comm_size(MCW, &size);
        if (rank == 0)printf("DinfDistUpToRidge -h version %s, modified by Liangjun Zhu, Apr 2, 2015\n", TDVERSION);

        float wt = 1.f; // is usew is 0, every grid's weight is 1.0
        float angle = 0.f, sump = 0.f, distr = 0.f, dtss = 0.f;
        double p; // to store ?

        //  Keep track of time
        double begint = MPI_Wtime();

        //Create tiff object, read and store header info
        tiffIO ang(angfile, FLOAT_TYPE);
        long totalX = ang.getTotalX();
        long totalY = ang.getTotalY();
        double dx = ang.getdxA();
        double dy = ang.getdyA();
        //if(rank==0)
        //{
        //	float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //	fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        //	fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        //	fflush(stderr);
        //}

        //  Calculate horizontal distances in each direction
        int kk;
        for (kk = 1; kk <= 8; kk++)
        {
            dist[kk] = sqrt((float)dx * (float)dx * d2[kk] * d2[kk] + (float)dy * (float)dy * d1[kk] * d1[kk]);
        }

        //Create partition and read data
        tdpartition *flowData;
        flowData = CreateNewPartition(ang.getDatatype(), totalX, totalY, dx, dy, ang.getNodata());
        int nx = flowData->getnx(); // nx is cols num.
        int ny = flowData->getny(); // ny is rows num. of current rank's partition
        int xstart, ystart;
        flowData->localToGlobal(0, 0, xstart, ystart);
        flowData->savedxdyc(ang);
        ang.read(xstart, ystart, ny, nx, flowData->getGridPointer()); // read current rank's partition data

        //if using ridgeData, get information from file, added by Zhu LJ, Apr 2,2015
        tdpartition *ridgeData;
        if (userdg)
        {
            tiffIO rdg(rdgfile, SHORT_TYPE);
            if (!ang.compareTiff(rdg))
            {
                printf("File size do not match\n%s\n", rdgfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            ridgeData = CreateNewPartition(rdg.getDatatype(), totalX, totalY, dx, dy, rdg.getNodata());
            rdg.read(xstart, ystart, ridgeData->getny(), ridgeData->getnx(), ridgeData->getGridPointer());
        }


        //if using weightData, get information from file
        tdpartition *weightData;
        if (usew)
        {
            tiffIO w(wfile, FLOAT_TYPE);
            if (!ang.compareTiff(w))
            {
                printf("File sizes do not match\n%s\n", wfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            weightData = CreateNewPartition(w.getDatatype(), totalX, totalY, dx, dy, w.getNodata());
            w.read(xstart, ystart, weightData->getny(), weightData->getnx(), weightData->getGridPointer());
        }

        //Begin timer
        double readt = MPI_Wtime();

        //Create empty partition to store new information (distance value)
        tdpartition *dts;
        dts = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, MISSINGFLOAT);

        // con is used to check for contamination at the edges
        long i, j;
        short k;
        long in, jn;
        bool con = false, finished;
        short tempShort = 0;

        tdpartition *neighbor;
        neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dx, dy, MISSINGSHORT);

        //Share information and set borders to zero
        flowData->share();
        if (usew) weightData->share();
        if (userdg) ridgeData->share();
        dts->share();  // to fill borders with no data
        neighbor->clearBorders();

        node temp;
        queue<node> que;

        //Count the flow receiving neighbors and put node with no contributing neighbors on que
        int useOutlets = 0;
        long numOutlets = 0;
        int *outletsX = 0, *outletsY = 0;
        initNeighborDinfup(neighbor, flowData, &que, nx, ny, useOutlets, outletsX, outletsY, numOutlets);

        finished = false;
        //Ring terminating while loop
        while (!finished)
        {
            while (!que.empty())
            {
                //Takes next node with no contributing neighbors
                temp = que.front();
                que.pop();
                i = temp.x;
                j = temp.y;
                //  EVALUATE UP FLOW ALGEBRA EXPRESSION
                distr = 0.f;  //  initialized at 0
                sump = 0.f;
                bool first = true;
                con = false;  // Start off not edge contaminated
                for (k = 1; k <= 8; k++)
                {
                    in = i + d1[k];
                    jn = j + d2[k];
                    if (!flowData->hasAccess(in, jn) || flowData->isNodata(in, jn))
                        con = true; // node temp is on the border
                    else
                    {
                        flowData->getData(in, jn, angle);
                        p = prop(angle, (k + 4) % 8, dx, dy);
                        if (p > 0. && p > thresh)
                        {
                            if (dts->isNodata(in, jn))con = true;
                            else
                            {
                                sump += p;
                                dts->getData(in, jn, dtss);
                                wt = 1.f;
                                if (usew)
                                {
                                    if (weightData->isNodata(in, jn))
                                        con = true;
                                    else
                                        weightData->getData(in, jn, wt);
                                }
                                if (statmethod == 0)
                                {//average
                                    distr += p * (dist[k] * wt + dtss);
                                }
                                else if (statmethod == 1)
                                {// maximum
                                    if (dist[k] * wt + dtss > distr)distr = dist[k] * wt + dtss;
                                }
                                else
                                { // Minimum
                                    if (first)
                                    {
                                        distr = dist[k] * wt + dtss;
                                        first = false;
                                    } else
                                    {
                                        if (dist[k] * wt + dtss < distr)distr = dist[k] * wt + dtss;
                                    }
                                }
                            }
                        }
                    }
                }
                if ((con && concheck))dts->setToNodata(i, j); // set to no data if contamination and checking
                else
                {
                    if (statmethod == 0 && sump > 0.f)dts->setData(i, j,  distr / sump);
                    else dts->setData(i, j, distr);
                }
                //  END UP FLOW ALGEBRA EVALUATION
                //  Decrement neighbor dependence of downslope cell
                flowData->getData(i, j, angle);
                for (k = 1; k <= 8; k++)
                {
                    p = prop(angle, k, dx, dy);
                    if (p > 0.0)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        //Decrement the number of contributing neighbors in neighbor
                        neighbor->addToData(in, jn, (short) -1);
                        //Check if neighbor needs to be added to que
                        if (flowData->isInPartition(in, jn) && neighbor->getData(in, jn, tempShort) == 0)
                        {
                            temp.x = in;
                            temp.y = jn;
                            que.push(temp);
                        }
                    }
                }
                short rdgornot;
                if (userdg)
                {
                    ridgeData->getData(i, j, rdgornot);
                    if (rdgornot == 1)
                    {
                        dts->setData(i, j, (float) 0.);
                    }
                }

            }

            //Pass information
            dts->share();
            neighbor->addBorders();

            //If this created a cell with no contributing neighbors, put it on the queue
            for (i = 0; i < nx; i++)
            {
                if (neighbor->getData(i, -1, tempShort) != 0 && neighbor->getData(i, 0, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = 0;
                    que.push(temp);
                }
                if (neighbor->getData(i, ny, tempShort) != 0 && neighbor->getData(i, ny - 1, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = ny - 1;
                    que.push(temp);
                }
            }

            neighbor->clearBorders();

            //Check if done
            finished = que.empty();
            finished = (bool)dts->ringTerm(finished);
        }

        //Stop timer
        double computet = MPI_Wtime();

        //Create and write TIFF file
        float ddNodata = MISSINGFLOAT;
        tiffIO dd(rtrfile, FLOAT_TYPE, ddNodata, ang);  // copy ang
        dd.write(xstart, ystart, ny, nx, dts->getGridPointer());

        double writet = MPI_Wtime();
        double dataRead, compute, write, total, tempd;
        dataRead = readt - begint;
        compute = computet - readt;
        write = writet - computet;
        total = writet - begint;

        MPI_Allreduce(&dataRead, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        dataRead = tempd;
        MPI_Allreduce(&compute, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        compute = tempd;
        MPI_Allreduce(&write, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        write = tempd;
        MPI_Allreduce(&total, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        total = tempd;

        if (rank == 0)
            printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                   size, dataRead, compute, write, total);

        //Brackets force MPI-dependent objects to go out of scope before Finalize is called

		/// free memory
		delete flowData;
		delete ridgeData;
		delete neighbor;
		delete dts;
    }
    MPI_Finalize();

    return 0;
}


//***************************//
//Vertical rise to the ridge //
//**************************//
int vrisetoridgegrd(char *angfile, char *felfile, char *rdgfile, char *rtrfile, int statmethod,
                    int concheck, float thresh, int userdg)
{
    MPI_Init(NULL, NULL);
    {

        //Only used for timing
        int rank, size;
        MPI_Comm_rank(MCW, &rank);
        MPI_Comm_size(MCW, &size);
        if (rank == 0)printf("DinfDistUpToRidge -v version %s\n", TDVERSION);

        float wt = 1.0, angle, sump, distr, dtss, elv, elvn, distk;
        double p;

        //  Keep track of time
        double begint = MPI_Wtime();

        //Create tiff object, read and store header info
        tiffIO ang(angfile, FLOAT_TYPE);
        long totalX = ang.getTotalX();
        long totalY = ang.getTotalY();
        double dx = ang.getdxA();
        double dy = ang.getdyA();
        //if(rank==0)
        //{
        //	float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //	fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        //	fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        //	fflush(stderr);
        //}

        //Create partition and read data
        tdpartition *flowData;
        flowData = CreateNewPartition(ang.getDatatype(), totalX, totalY, dx, dy, ang.getNodata());
        int nx = flowData->getnx();
        int ny = flowData->getny();
        int xstart, ystart;
        flowData->localToGlobal(0, 0, xstart, ystart);
        flowData->savedxdyc(ang);
        ang.read(xstart, ystart, ny, nx, flowData->getGridPointer());

        //if using ridgeData, get information from file, added by Zhu LJ, Apr 2,2015
        tdpartition *ridgeData;
        if (userdg)
        {
            tiffIO rdg(rdgfile, SHORT_TYPE);
            if (!ang.compareTiff(rdg))
            {
                printf("File size do not match\n%s\n", rdgfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            ridgeData = CreateNewPartition(rdg.getDatatype(), totalX, totalY, dx, dy, rdg.getNodata());
            rdg.read(xstart, ystart, ridgeData->getny(), ridgeData->getnx(), ridgeData->getGridPointer());
        }
        //  Elevation data
        tdpartition *felData;
        tiffIO fel(felfile, FLOAT_TYPE);
        if (!ang.compareTiff(fel))
        {
            printf("File sizes do not match\n%s\n", felfile);
            MPI_Abort(MCW, 5);
            return 1;
        }
        felData = CreateNewPartition(fel.getDatatype(), totalX, totalY, dx, dy, fel.getNodata());
        fel.read(xstart, ystart, felData->getny(), felData->getnx(), felData->getGridPointer());

        //Begin timer
        double readt = MPI_Wtime();

        //Create empty partition to store new information
        tdpartition *dts;
        dts = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, MISSINGFLOAT);

        // con is used to check for contamination at the edges
        long i, j;
        short k;
        long in, jn;
        bool con = false, finished;
//        float tempFloat = 0;
        short tempShort = 0;

        tdpartition *neighbor;
        neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dx, dy, MISSINGSHORT);

        //Share information and set borders to zero
        flowData->share();
        felData->share();
        if (userdg) ridgeData->share();
        dts->share();  // to fill borders with no data
        neighbor->clearBorders();

        node temp;
        queue<node> que;

        //Count the flow receiving neighbors and put on queue
        int useOutlets = 0;
        long numOutlets = 0;
        int *outletsX = 0, *outletsY = 0;
        initNeighborDinfup(neighbor, flowData, &que, nx, ny, useOutlets, outletsX, outletsY, numOutlets);

        finished = false;
        //Ring terminating while loop
        while (!finished)
        {
            while (!que.empty())
            {
                //Takes next node with no contributing neighbors
                temp = que.front();
                que.pop();
                i = temp.x;
                j = temp.y;
                //  EVALUATE UP FLOW ALGEBRA EXPRESSION
                distr = 0.f;  //  initialized at 0
                sump = 0.f;
                bool first = true;
                felData->getData(i, j, elv);
                con = false;  // Start off not edge contaminated
                for (k = 1; k <= 8; k++)
                {
                    in = i + d1[k];
                    jn = j + d2[k];
                    if (!flowData->hasAccess(in, jn) || flowData->isNodata(in, jn))
                        con = true;
                    else
                    {
                        flowData->getData(in, jn, angle);
                        p = prop(angle, (k + 4) % 8, dx, dy);
                        if (p > 0. && p > thresh)
                        {
                            if (dts->isNodata(in, jn))con = true;
                            else if (felData->isNodata(in, jn))con = true;
                            else
                            {
                                sump += p;
                                dts->getData(in, jn, dtss);
                                felData->getData(in, jn, elvn);
                                distk = elvn - elv;
                                wt = 1.f;
                                //if(usew==1){
                                //	if(weightData->isNodata(in,jn))
                                //		con=true;
                                //	else
                                //		weightData->getData(in,jn,wt);
                                //}
                                if (statmethod == 0)
                                {//average
                                    distr += p * (distk * wt + dtss);
                                }
                                else if (statmethod == 1)
                                {// maximum
                                    if (first)
                                    {  //  do not assume that maximum elevation diff is positive in case of wierd (or not pit filled) elevations
                                        distr = distk * wt + dtss;
                                        first = false;
                                    } else
                                    {
                                        if (distk * wt + dtss > distr)distr = distk * wt + dtss;
                                    }
                                }
                                else
                                { // Minimum
                                    if (first)
                                    {
                                        distr = distk * wt + dtss;
                                        first = false;
                                    } else
                                    {
                                        if (distk * wt + dtss < distr)distr = distk * wt + dtss;
                                    }
                                }
                            }
                        }
                    }
                }
                if ((con && concheck == 1))dts->setToNodata(i, j); // set to no data if contamination and checking
                else
                {
                    if (statmethod == 0 && sump > 0.)dts->setData(i, j,  distr / sump);
                    else dts->setData(i, j, distr);
                }
                //  END UP FLOW ALGEBRA EVALUATION
                //  Decrement neighbor dependence of downslope cell
                flowData->getData(i, j, angle);
                for (k = 1; k <= 8; k++)
                {
                    p = prop(angle, k, dx, dy);
                    if (p > 0.0)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        //Decrement the number of contributing neighbors in neighbor
                        neighbor->addToData(in, jn, (short) -1);
                        //Check if neighbor needs to be added to que
                        if (flowData->isInPartition(in, jn) && neighbor->getData(in, jn, tempShort) == 0)
                        {
                            temp.x = in;
                            temp.y = jn;
                            que.push(temp);
                        }
                    }
                }
                short rdgornot;
                if (userdg)
                {
                    ridgeData->getData(i, j, rdgornot);
                    if (rdgornot == 1)
                    {
                        dts->setData(i, j, 0.f);
                    }
                }
            }

            //Pass information
            dts->share();
            neighbor->addBorders();

            //If this created a cell with no contributing neighbors, put it on the queue
            for (i = 0; i < nx; i++)
            {
                if (neighbor->getData(i, -1, tempShort) != 0 && neighbor->getData(i, 0, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = 0;
                    que.push(temp);
                }
                if (neighbor->getData(i, ny, tempShort) != 0 && neighbor->getData(i, ny - 1, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = ny - 1;
                    que.push(temp);
                }
            }

            neighbor->clearBorders();

            //Check if done
            finished = que.empty();
            finished = (bool)dts->ringTerm(finished);
        }

        //Stop timer
        double computet = MPI_Wtime();

        //Create and write TIFF file
        float ddNodata = MISSINGFLOAT;
        tiffIO dd(rtrfile, FLOAT_TYPE, ddNodata, ang);
        dd.write(xstart, ystart, ny, nx, dts->getGridPointer());

        double writet = MPI_Wtime();
        double dataRead, compute, write, total, tempd;
        dataRead = readt - begint;
        compute = computet - readt;
        write = writet - computet;
        total = writet - begint;

        MPI_Allreduce(&dataRead, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        dataRead = tempd;
        MPI_Allreduce(&compute, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        compute = tempd;
        MPI_Allreduce(&write, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        write = tempd;
        MPI_Allreduce(&total, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        total = tempd;

        if (rank == 0)
            printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                   size, dataRead, compute, write, total);

        //Brackets force MPI-dependent objects to go out of scope before Finalize is called
    }
    MPI_Finalize();

    return 0;
}

//*********************************//
//Pythagoras distance to the ridge //
//********************************//
int pdisttoridgegrd(char *angfile, char *felfile, char *rdgfile, char *wfile, char *rtrfile,
                    int statmethod, int userdg, int usew, int concheck, float thresh)
{
    MPI_Init(NULL, NULL);
    {

        //Only used for timing
        int rank, size;
        MPI_Comm_rank(MCW, &rank);
        MPI_Comm_size(MCW, &size);
        if (rank == 0)printf("DinfDistUpToRidge -p version %s\n", TDVERSION);

        float wt = 1.0, angle, sump, distrh, distrv, dtssh, dtssv, elvn, elv, distk;
        double p;

        //  Keep track of time
        double begint = MPI_Wtime();

        //Create tiff object, read and store header info
        tiffIO ang(angfile, FLOAT_TYPE);
        long totalX = ang.getTotalX();
        long totalY = ang.getTotalY();
        double dx = ang.getdxA();
        double dy = ang.getdyA();
        //if(rank==0)
        //{
        //	float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //	fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        //	fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        //	fflush(stderr);
        //}

        //  Calculate horizontal distances in each direction
        int kk;
        for (kk = 1; kk <= 8; kk++)
        {
            dist[kk] = sqrt((float)dx * (float)dx * d2[kk] * d2[kk] + (float)dy * (float)dy * d1[kk] * d1[kk]);
        }

        //Create partition and read data
        tdpartition *flowData;
        flowData = CreateNewPartition(ang.getDatatype(), totalX, totalY, dx, dy, ang.getNodata());
        int nx = flowData->getnx();
        int ny = flowData->getny();
        int xstart, ystart;
        flowData->localToGlobal(0, 0, xstart, ystart);
        flowData->savedxdyc(ang);
        ang.read(xstart, ystart, ny, nx, flowData->getGridPointer());

        //if using ridgeData, get information from file, added by Zhu LJ, Apr 2,2015
        tdpartition *ridgeData;
        if (userdg)
        {
            tiffIO rdg(rdgfile, SHORT_TYPE);
            if (!ang.compareTiff(rdg))
            {
                printf("File size do not match\n%s\n", rdgfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            ridgeData = CreateNewPartition(rdg.getDatatype(), totalX, totalY, dx, dy, rdg.getNodata());
            rdg.read(xstart, ystart, ridgeData->getny(), ridgeData->getnx(), ridgeData->getGridPointer());
        }

        //  Elevation data
        tdpartition *felData;
        tiffIO fel(felfile, FLOAT_TYPE);
        if (!ang.compareTiff(fel))
        {
            printf("File sizes do not match\n%s\n", felfile);
            MPI_Abort(MCW, 5);
            return 1;
        }
        felData = CreateNewPartition(fel.getDatatype(), totalX, totalY, dx, dy, fel.getNodata());
        fel.read(xstart, ystart, felData->getny(), felData->getnx(), felData->getGridPointer());

        //if using weightData, get information from file
        tdpartition *weightData;
        if (usew == 1)
        {
            tiffIO w(wfile, FLOAT_TYPE);
            if (!ang.compareTiff(w))
            {
                printf("File sizes do not match\n%s\n", wfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            weightData = CreateNewPartition(w.getDatatype(), totalX, totalY, dx, dy, w.getNodata());
            w.read(xstart, ystart, weightData->getny(), weightData->getnx(), weightData->getGridPointer());
        }

        //Begin timer
        double readt = MPI_Wtime();

        //Create empty partitions to store new information
        tdpartition *dtsh;  // horizontal distance
        dtsh = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, MISSINGFLOAT);

        tdpartition *dtsv;  // vertical distance
        dtsv = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, MISSINGFLOAT);

        // con is used to check for contamination at the edges
        long i, j;
        short k;
        long in, jn;
        bool con = false, finished;
//        float tempFloat = 0;
        short tempShort = 0;

        tdpartition *neighbor;
        neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dx, dy, MISSINGSHORT);

        //Share information and set borders to zero
        flowData->share();
        felData->share();
        if (usew == 1) weightData->share();
        if (userdg) ridgeData->share();
        dtsh->share();  // to fill borders with no data
        dtsv->share();
        neighbor->clearBorders();

        node temp;
        queue<node> que;

        //Count the flow receiving neighbors and put on queue
        int useOutlets = 0;
        long numOutlets = 0;
        int *outletsX = 0, *outletsY = 0;
        initNeighborDinfup(neighbor, flowData, &que, nx, ny, useOutlets, outletsX, outletsY, numOutlets);

        finished = false;
        //Ring terminating while loop
        while (!finished)
        {
            while (!que.empty())
            {
                //Takes next node with no contributing neighbors
                temp = que.front();
                que.pop();
                i = temp.x;
                j = temp.y;
                //  EVALUATE UP FLOW ALGEBRA EXPRESSION
                if (felData->isNodata(i, j))
                {
                    dtsv->setToNodata(i, j);  //  If elevation is not known result has to be no data
                    dtsh->setToNodata(i, j);
                }
                else
                {
                    //distrh=0.0;  // distance result
                    distrh = 0.5f * sqrt(2.f);
                    distrv = 0.f;
                    sump = 0.f;
                    bool first = true;
                    felData->getData(i, j, elv);
                    con = false;  // Start off not edge contaminated
                    for (k = 1; k <= 8; k++)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (!flowData->hasAccess(in, jn) || flowData->isNodata(in, jn))
                            con = true;
                        else
                        {
                            flowData->getData(in, jn, angle);
                            p = prop(angle, (k + 4) % 8, dx, dy);
                            if (p > 0. && p > thresh)
                            {
                                if (dtsh->isNodata(in, jn))con = true;
                                else if (felData->isNodata(in, jn))con = true;
                                else
                                {
                                    sump += p;
                                    dtsh->getData(in, jn, dtssh);
                                    dtsv->getData(in, jn, dtssv);
                                    felData->getData(in, jn, elvn);
                                    distk = elvn - elv;
                                    wt = 1.;
                                    if (usew == 1)
                                    {
                                        if (weightData->isNodata(in, jn))
                                            con = true;
                                        else
                                            weightData->getData(in, jn, wt);
                                    }
                                    if (statmethod == 0)
                                    {//average
                                        distrh += p * (dist[k] * wt + dtssh);
                                        distrv += p * (distk + dtssv);
                                    }
                                    else if (statmethod == 1)
                                    {// maximum
                                        if (first)
                                        {  //  do not assume that maximum elevation diff is positive in case of wierd (or not pit filled) elevations
                                            distrh = dist[k] * wt + dtssh;
                                            distrv = distk + dtssv;
                                            first = false;
                                        } else
                                        {
                                            if (dist[k] * wt + dtssh > distrh)distrh = dist[k] * wt + dtssh;
                                            if (distk + dtssv > distrv)distrv = distk + dtssv;
                                        }
                                    }
                                    else
                                    { // Minimum
                                        if (first)
                                        {
                                            distrh = dist[k] * wt + dtssh;
                                            distrv = distk + dtssv;
                                            first = false;
                                        } else
                                        {
                                            if (dist[k] * wt + dtssh < distrh)distrh = dist[k] * wt + dtssh;
                                            if (distk + dtssv < distrv)distrv = distk + dtssv;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if ((con && concheck == 1))// set to no data if contamination and checking
                    {
                        dtsh->setToNodata(i, j);
                        dtsv->setToNodata(i, j);
                    }
                    else
                    {
                        if (statmethod == 0 && sump > 0.)
                        {
                            dtsh->setData(i, j,  distrh / sump);
                            dtsv->setData(i, j,  distrv / sump);
                        }
                        else
                        {
                            dtsh->setData(i, j, distrh);
                            dtsv->setData(i, j, distrv);
                        }
                    }
                }
                //  END UP FLOW ALGEBRA EVALUATION
                //  Decrement neighbor dependence of downslope cell
                flowData->getData(i, j, angle);
                for (k = 1; k <= 8; k++)
                {
                    p = prop(angle, k, dx, dy);
                    if (p > 0.0)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        //Decrement the number of contributing neighbors in neighbor
                        neighbor->addToData(in, jn, (short) -1);
                        //Check if neighbor needs to be added to que
                        if (flowData->isInPartition(in, jn) && neighbor->getData(in, jn, tempShort) == 0)
                        {
                            temp.x = in;
                            temp.y = jn;
                            que.push(temp);
                        }
                    }
                }
                short rdgornot;
                if (userdg)
                {
                    ridgeData->getData(i, j, rdgornot);
                    if (rdgornot == 1)
                    {
                        dtsh->setData(i, j, 0.f);
                        dtsv->setData(i, j, 0.f);
                    }
                }
            }

            //Pass information
            dtsh->share();
            dtsv->share();
            neighbor->addBorders();

            //If this created a cell with no contributing neighbors, put it on the queue
            for (i = 0; i < nx; i++)
            {
                if (neighbor->getData(i, -1, tempShort) != 0 && neighbor->getData(i, 0, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = 0;
                    que.push(temp);
                }
                if (neighbor->getData(i, ny, tempShort) != 0 && neighbor->getData(i, ny - 1, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = ny - 1;
                    que.push(temp);
                }
            }

            neighbor->clearBorders();

            //Check if done
            finished = que.empty();
            finished = (bool)dtsh->ringTerm(finished);
        }

        //  Now compute the pythagorus difference
        for (j = 0; j < ny; j++)
        {
            for (i = 0; i < nx; i++)
            {
                if (dtsv->isNodata(i, j))dtsh->setToNodata(i, j);
                else if (!dtsh->isNodata(i, j))
                {
                    dtsh->getData(i, j, dtssh);
                    dtsv->getData(i, j, dtssv);
                    dtssh = sqrt(dtssh * dtssh + dtssv * dtssv);
                    dtsh->setData(i, j, dtssh);
                }
            }
        }


        //Stop timer
        double computet = MPI_Wtime();

        //Create and write TIFF file
        float ddNodata = MISSINGFLOAT;
        tiffIO dd(rtrfile, FLOAT_TYPE, ddNodata, ang);
        dd.write(xstart, ystart, ny, nx, dtsh->getGridPointer());

        double writet = MPI_Wtime();
        double dataRead, compute, write, total, tempd;
        dataRead = readt - begint;
        compute = computet - readt;
        write = writet - computet;
        total = writet - begint;

        MPI_Allreduce(&dataRead, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        dataRead = tempd;
        MPI_Allreduce(&compute, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        compute = tempd;
        MPI_Allreduce(&write, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        write = tempd;
        MPI_Allreduce(&total, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
        total = tempd;

        if (rank == 0)
            printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                   size, dataRead, compute, write, total);

        //Brackets force MPI-dependent objects to go out of scope before Finalize is called
    }
    MPI_Finalize();

    return 0;
}


//******************************//
//Surface distance to the ridge //
//*****************************//
int sdisttoridgegrd(char *angfile, char *felfile, char *rdgfile, char *wfile, char *rtrfile,
                    int statmethod, int userdg, int usew, int concheck, float thresh)
{
    MPI_Init(NULL, NULL);
    {

        //Only used for timing
        int rank, size;
        MPI_Comm_rank(MCW, &rank);
        MPI_Comm_size(MCW, &size);
        if (rank == 0)printf("DinfDistUpToRidge -s version %s\n", TDVERSION);

        float wt = 1.0, angle, sump, distr, dtss, elvn, elv, distk;
        double p;

        //  Keep track of time
        double begint = MPI_Wtime();

        //Create tiff object, read and store header info
        tiffIO ang(angfile, FLOAT_TYPE);
        long totalX = ang.getTotalX();
        long totalY = ang.getTotalY();
        double dx = ang.getdxA();
        double dy = ang.getdyA();
        //if(rank==0)
        //{
        //	float timeestimate=(1.2e-6*totalX*totalY/pow((double) size,0.65))/60+1;  // Time estimate in minutes
        //	fprintf(stderr,"This run may take on the order of %.0f minutes to complete.\n",timeestimate);
        //	fprintf(stderr,"This estimate is very approximate. \nRun time is highly uncertain as it depends on the complexity of the input data \nand speed and memory of the computer. This estimate is based on our testing on \na dual quad core Dell Xeon E5405 2.0GHz PC with 16GB RAM.\n");
        //	fflush(stderr);
        //}

        //  Calculate horizontal distances in each direction
        int kk; // loop eight directions
        for (kk = 1; kk <= 8; kk++)
        {
            dist[kk] = sqrt((float)dx * (float)dx * d2[kk] * d2[kk] + (float)dy * (float)dy * d1[kk] * d1[kk]);
        }

        //Create partition and read data
        tdpartition *flowData;
        flowData = CreateNewPartition(ang.getDatatype(), totalX, totalY, dx, dy, ang.getNodata());
        int nx = flowData->getnx();
        int ny = flowData->getny();
        int xstart, ystart;
        flowData->localToGlobal(0, 0, xstart, ystart);
        flowData->savedxdyc(ang);
        ang.read(xstart, ystart, ny, nx, flowData->getGridPointer());

        //if using ridgeData, get information from file, added by Zhu LJ, Apr 2,2015
        tdpartition *ridgeData;
        if (userdg)
        {
            tiffIO rdg(rdgfile, SHORT_TYPE);
            if (!ang.compareTiff(rdg))
            {
                printf("File size do not match\n%s\n", rdgfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            ridgeData = CreateNewPartition(rdg.getDatatype(), totalX, totalY, dx, dy, rdg.getNodata());
            rdg.read(xstart, ystart, ridgeData->getny(), ridgeData->getnx(), ridgeData->getGridPointer());
        }
        //  Elevation data
        tdpartition *felData;
        tiffIO fel(felfile, FLOAT_TYPE);
        if (!ang.compareTiff(fel))
        {
            printf("File sizes do not match\n%s\n", felfile);
            MPI_Abort(MCW, 5);
            return 1;
        }
        felData = CreateNewPartition(fel.getDatatype(), totalX, totalY, dx, dy, fel.getNodata());
        fel.read(xstart, ystart, felData->getny(), felData->getnx(), felData->getGridPointer());

        //if using weightData, get information from file
        tdpartition *weightData;
        if (usew == 1)
        {
            tiffIO w(wfile, FLOAT_TYPE);
            if (!ang.compareTiff(w))
            {
                printf("File sizes do not match\n%s\n", wfile);
                MPI_Abort(MCW, 5);
                return 1;
            }
            weightData = CreateNewPartition(w.getDatatype(), totalX, totalY, dx, dy, w.getNodata());
            w.read(xstart, ystart, weightData->getny(), weightData->getnx(), weightData->getGridPointer());
        }

        //Begin timer
        double readt = MPI_Wtime();

        //Create empty partitions to store new information
        tdpartition *dts;  // surface distance
        dts = CreateNewPartition(FLOAT_TYPE, totalX, totalY, dx, dy, MISSINGFLOAT);

        // con is used to check for contamination at the edges
        long i, j;
        short k;
        long in, jn;
        bool con = false, finished;
//        float tempFloat = 0;
        short tempShort = 0;

        tdpartition *neighbor;
        neighbor = CreateNewPartition(SHORT_TYPE, totalX, totalY, dx, dy, MISSINGSHORT);

        //Share information and set borders to zero
        flowData->share();
        felData->share();
        if (usew == 1) weightData->share();
        if (userdg) ridgeData->share();
        dts->share();
        neighbor->clearBorders();

        node temp;
        queue<node> que;

        //Count the flow receiving neighbors and put on queue
        int useOutlets = 0;
        long numOutlets = 0;
        int *outletsX = 0, *outletsY = 0;
        initNeighborDinfup(neighbor, flowData, &que, nx, ny, useOutlets, outletsX, outletsY, numOutlets);

        finished = false;
        //Ring terminating while loop
        while (!finished)
        {
            while (!que.empty())
            {
                //Takes next node with no contributing neighbors
                temp = que.front();
                que.pop();
                i = temp.x;
                j = temp.y;
                //  EVALUATE UP FLOW ALGEBRA EXPRESSION
                if (felData->isNodata(i, j))
                {
                    dts->setToNodata(i, j);  //  If elevation is not known result has to be no data
                }
                else
                {
                    distr = 0.f;  // distance result
                    sump = 0.f;
                    bool first = true;
                    felData->getData(i, j, elv);
                    con = false;  // Start off not edge contaminated
                    for (k = 1; k <= 8; k++)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        if (!flowData->hasAccess(in, jn) || flowData->isNodata(in, jn))
                            con = true;
                        else
                        {
                            flowData->getData(in, jn, angle);
                            p = prop(angle, (k + 4) % 8, dx, dy);
                            if (p > 0. && p > thresh)
                            {
                                if (dts->isNodata(in, jn))con = true;
                                else if (felData->isNodata(in, jn))con = true;
                                else
                                {
                                    sump += p;
                                    dts->getData(in, jn, dtss);
                                    felData->getData(in, jn, elvn);
                                    wt = 1.f;
                                    if (usew == 1)
                                    {
                                        if (weightData->isNodata(in, jn))
                                            con = true;
                                        else
                                            weightData->getData(in, jn, wt);
                                    }
                                    distk = sqrt((elv - elvn) * (elv - elvn) + (dist[k] * wt) * (dist[k] * wt));
                                    if (statmethod == 0)
                                    {//average
                                        distr += p * (distk + dtss);
                                    }
                                    else if (statmethod == 1)
                                    {// maximum
                                        if (first)
                                        {  //  do not assume that maximum elevation diff is positive in case of wierd (or not pit filled) elevations
                                            distr = distk + dtss;
                                            first = false;
                                        } else
                                        {
                                            if (distk + dtss > distr)distr = distk + dtss;
                                        }
                                    }
                                    else
                                    { // Minimum
                                        if (first)
                                        {
                                            distr = distk + dtss;
                                            first = false;
                                        } else
                                        {
                                            if (distk + dtss < distr)distr = distk + dtss;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if ((con && concheck == 1))// set to no data if contamination and checking
                    {
                        dts->setToNodata(i, j);
                    }
                    else
                    {
                        if (statmethod == 0 && sump > 0.)
                        {
                            dts->setData(i, j, (float) (distr / sump));
                        }
                        else
                        {
                            dts->setData(i, j, distr);
                        }
                    }
                }
                //  END UP FLOW ALGEBRA EVALUATION
                //  Decrement neighbor dependence of downslope cell
                flowData->getData(i, j, angle);
                for (k = 1; k <= 8; k++)
                {
                    p = prop(angle, k, dx, dy);
                    if (p > 0.0)
                    {
                        in = i + d1[k];
                        jn = j + d2[k];
                        //Decrement the number of contributing neighbors in neighbor
                        neighbor->addToData(in, jn, (short) -1);
                        //Check if neighbor needs to be added to que
                        if (flowData->isInPartition(in, jn) && neighbor->getData(in, jn, tempShort) == 0)
                        {
                            temp.x = in;
                            temp.y = jn;
                            que.push(temp);
                        }
                    }
                }
                short rdgornot;
                if (userdg)
                {
                    ridgeData->getData(i, j, rdgornot);
                    if (rdgornot == 1)
                    {
                        dts->setData(i, j, 0.f);
                    }
                }
            }

            //Pass information
            dts->share();
            neighbor->addBorders();

            //If this created a cell with no contributing neighbors, put it on the queue
            for (i = 0; i < nx; i++)
            {
                if (neighbor->getData(i, -1, tempShort) != 0 && neighbor->getData(i, 0, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = 0;
                    que.push(temp);
                }
                if (neighbor->getData(i, ny, tempShort) != 0 && neighbor->getData(i, ny - 1, tempShort) == 0)
                {
                    temp.x = i;
                    temp.y = ny - 1;
                    que.push(temp);
                }
            }

            neighbor->clearBorders();

            //Check if done
            finished = que.empty();
            finished = (bool)dts->ringTerm(finished);
        }

        //Stop timer
        double computet = MPI_Wtime();

        //Create and write TIFF file
        float ddNodata = MISSINGFLOAT;
        tiffIO dd(rtrfile, FLOAT_TYPE, ddNodata, ang);
        dd.write(xstart, ystart, ny, nx, dts->getGridPointer());

        double writet = MPI_Wtime();
        double dataRead, compute, write, total, tempd;
        dataRead = readt - begint;
        compute = computet - readt;
        write = writet - computet;
        total = writet - begint;

		MPI_Allreduce(&dataRead, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
		dataRead = tempd;
		MPI_Allreduce(&compute, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
		compute = tempd;
		MPI_Allreduce(&write, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
		write = tempd;
		MPI_Allreduce(&total, &tempd, 1, MPI_DOUBLE, MPI_MAX, MCW);
		total = tempd;

        if (rank == 0)
            printf("Processors: %d\nRead time: %f\nCompute time: %f\nWrite time: %f\nTotal time: %f\n",
                   size, dataRead, compute, write, total);

        //Brackets force MPI-dependent objects to go out of scope before Finalize is called
    }
    MPI_Finalize();

    return 0;
}
