
## Usage

```
docker pull ghcr.io/lreis2415/taudem_ext:alpine
cd TauDEM_ext
mkdir -p data/results
#
# TauDEM cmd: docker run -v $(pwd)/data:/data ghcr.io/lreis2415/taudem_ext:alpine [-np <nproc>] <TauDEMToolName> <Arguments>
# where, nproc is the number of MPI processes, the default is the number of processors assigned to the Docker, the "-np <nproc>" is optional.
#
# For example,
# 1. pit remove
docker run -v $(pwd)/data:/data ghcr.io/lreis2415/taudem_ext:alpine pitremove -z /data/logan.tif -fel /data/results/loganfel.tif
# or specify -np 2
docker run -v $(pwd)/data:/data ghcr.io/lreis2415/taudem_ext:alpine -np 2 pitremove -z /data/logan.tif -fel /data/results/loganfel.tif
# 2. d8 flow direction
docker run -v $(pwd)/data:/data ghcr.io/lreis2415/taudem_ext:alpine d8flowdir -fel /data/results/loganfel.tif -p /data/results/logand8.tif
```

## Test for developers

```
cd TauDEM_ext
docker build -t taudem_ext:alpine -f docker/Dockerfile.alpine .
```
