#### Couche de base

FROM debian:bullseye-slim as libs

# Librairies

RUN apt update && apt -y install git gettext \
    libfcgi-dev \
    libtinyxml-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    libproj-dev \
    libssl-dev \
    libturbojpeg0-dev \
    libjpeg-dev \
    libc6-dev \
    libjson11-1-dev \
    libboost-log-dev libboost-filesystem-dev libboost-system-dev

#### Compilation de l'application

FROM libs AS builder

# Environnement de compilation

RUN apt -y install  \
    build-essential cmake

# Compilation et installation

ARG VERSION

COPY ./CMakeLists.txt /sources/CMakeLists.txt
COPY ./cmake /sources/cmake
COPY ./lib /sources/lib
COPY ./src /sources/src
COPY ./service /sources/service
COPY ./config.h.in /sources/config.h.in
COPY ./config /sources/config

RUN mkdir -p /build
WORKDIR /build

RUN cmake -DCMAKE_INSTALL_PREFIX=/ -DBUILD_VERSION=$VERSION -DOBJECT_ENABLED=0 /sources/
RUN make && make install

#### Image de run à partir des libs et de l'exécutable compilé

FROM libs

# Configuration par variables d'environnement par défaut
ENV IMPORT_LAYERS_FROM_PYRAMIDS=""
ENV SERVER_LOGLEVEL="error" SERVER_NBTHREAD="4" SERVER_CACHE_SIZE="1000" SERVER_CACHE_VALIDITY="10"
ENV SERVICE_TITLE="WMS/WMTS/TMS server"  SERVICE_ABSTRACT="This server provide WMS, WMTS and TMS raster and vector data broadcast"  SERVICE_PROVIDERNAME="ROK4 Team" SERVICE_PROVIDERSITE="https://github.com/rok4/rok4" SERVICE_KEYWORDS="WMS,WMTS,TMS,Docker"
ENV SERVICE_FEE="none" SERVICE_ACCESSCONSTRAINT="none"
ENV SERVICE_WMS="WMS service" SERVICE_MAXWIDTH="10000" SERVICE_MAXHEIGHT="10000" SERVICE_LAYERLIMIT="2" SERVICE_MAXTILEX="256" SERVICE_MAXTILEY="256" SERVICE_FORMATLIST="image/jpeg,image/png,image/tiff,image/geotiff,image/x-bil;bits=32"
ENV SERVICE_GLOBALCRSLIST="CRS:84,EPSG:3857" SERVICE_FULLYSTYLING="true" SERVICE_INSPIRE="false"
ENV SERVICE_WMTSSUPPORT="true" SERVICE_TMSSUPPORT="true" SERVICE_WMSSUPPORT="true"
ENV SERVICE_WMTS_ENDPOINT="http://localhost/wmts" SERVICE_TMS_ENDPOINT="http://localhost/tms" SERVICE_WMS_ENDPOINT="http://localhost/wms"

WORKDIR /

# Récupération de l'exécutable
COPY --from=builder /bin/rok4 /bin/rok4

# Déploiement des configurations
COPY ./docker/server.template.json /etc/rok4/config/server.template.json
COPY ./docker/services.template.json /etc/rok4/config/services.template.json

RUN git clone http://gitlab.forge-geoportail.ign.fr/rok4/styles.git /styles
RUN git clone http://gitlab.forge-geoportail.ign.fr/rok4/tilematrixsets.git /tilematrixsets

COPY ./docker/docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

RUN mkdir /layers /pyramids

VOLUME /layers
VOLUME /pyramids

EXPOSE 9000

ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["/bin/rok4", "-f", "/etc/rok4/config/server.json"]