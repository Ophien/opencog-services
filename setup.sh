#!/bin/bash

# configuration
OC_SERVICES_PORT=7031
RELEX_PORT=7034

# remove old images and containers
docker rm -f opencog_services_relex_container
docker rm -f opencog_services_container
docker rmi opencog_services_relex_image
docker rmi opencog_services_image

# build dependencies
docker build -t opencog_services_relex_image -f ./dependencies/dockerfiles/OpenCogRelex .
docker build -t opencog_services_image -f ./dependencies/dockerfiles/OpenCogServices .

# setup network
docker network rm opencog_services_network
docker network create opencog_services_network

# run containers
docker run --name opencog_services_relex_container \
           --restart unless-stopped \
           --network opencog_services_network \
           -p $RELEX_PORT:$4444 \
           -di opencog_services_relex_image /bin/sh opencog-server.sh

docker run --name opencog_services_container \
           --env RELEX_CONTAINER_NAME=opencog_services_relex_container \
           --env OPENCOG_SERVER_PORT=$OC_SERVICES_PORT \
           --restart unless-stopped \
           --network opencog_services_network \
           -p $OC_SERVICES_PORT:$OC_SERVICES_PORT \
           -di opencog_services_image ./bin/server