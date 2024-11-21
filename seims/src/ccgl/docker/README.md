
## Usage

TODO

## Test for developers
The <tag> can be `apline`, `apline-with-gdal`, `apline-with-mongodb`, and `apline-with-gdal-mongodb`.
```
docker build -t crazyzlj/ccgl:<tag>-test -f docker/test/<tag>/Dockerfile .

# or add `--progress=plain` to print all output from shell

docker build --progress=plain -t crazyzlj/ccgl:<tag>-test -f docker/test/<tag>/Dockerfile .
```

### CCGL with the support of MongoDB

See the [official tutorial: Docker and MongoDB](https://www.mongodb.com/resources/products/compatibilities/docker).

+ Select a proper tag of the official [mongodb-community-server image](https://hub.docker.com/r/mongodb/mongodb-community-server), e.g., `mongodb/mongodb-community-server:4.4.28-ubuntu2004`.
+ Create a docker network for the MongoDB server and other docker applications who want use this MongoDB: `docker network create docker-mongo-network`
+ Start a mongodb server under the above docker network: `docker run -d --rm --name mongodb-docker --network docker-mongo-network  -p <client_port>:27017 -v </path/to/store/mongodb/data>:/data/db mongodb/mongodb-community-server:4.4.28-ubuntu2004 mongod --bind_ip localhost,mongodb-docker`
  + `</path/to/store/mongodb/data>` means the local directory (path in English and without spaces) to store MongoDB data
  + `client_port` is the port you want to exposed to your localhost, e.g., `27017` or any other legal number.
  + Run `docker logs mongodb-docker` to check the logs which provide a wealth of useful information.
  + Now, you can connect to the MongoDB running in the container from *the host* host via `localhost:<client_port>` (e.g., using Robo 3T, VSCode with MongoDB plugin).
+ Check the IP address of the MongoDB container within the above defined docker network: `docker inspect network docker-mongo-network`, e.g., `172.18.0.2`
+ Run the `ccgl` container within the same docker network and pass the IP address as arguments: `docker run --rm -v $PWD/data:/data --net docker-mongo-network registry.cn-hangzhou.aliyuncs.com/ljzhu-geomodels/ccgl:dev-alpine-with-gdal-mongodb mask_rasterio -in /data/raster/dem_1.tif -out gfs dem_1 -mongo 172.18.0.2 27017 test spatial`.

> References:
> + [Connect to MongoDB from another Docker container](https://github.com/docker-library/docs/blob/master/mongo/README.md#connect-to-mongodb-from-another-docker-container).
> + An useful answer in [stackflow](https://stackoverflow.com/a/43962099).

## Release docker images

The <tag> can be `apline`, `apline-with-gdal`, `apline-with-mongodb`, and `apline-with-gdal-mongodb`.
The <ver> can be `latest` or any others, e.g., `v1.0`.
```
cd CCGL
docker buildx create --use
docker buildx build --platform linux/amd64,linux/arm64 --push -t crazyzlj/ccgl:<tag>-<ver> -f docker/<tag>/Dockerfile .
```
