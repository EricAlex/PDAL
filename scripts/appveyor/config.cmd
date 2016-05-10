@echo on

git clone https://github.com/hobu/laz-perf.git
cd laz-perf
cmake -G "Visual Studio 14 2015 Win64" -DCMAKE_INSTALL_PREFIX="%cd%/install" .
cmake --build . --target install
cd ..

cmake -G "Visual Studio 14 2015 Win64" ^
    -DBUILD_PLUGIN_CPD=OFF ^
    -DBUILD_PLUGIN_GREYHOUND=OFF ^
    -DBUILD_PLUGIN_HEXBIN=OFF ^
    -DBUILD_PLUGIN_ICEBRIDGE=OFF ^
    -DBUILD_PLUGIN_MRSID=OFF ^
    -DBUILD_PLUGIN_NITF=OFF ^
    -DBUILD_PLUGIN_OCI=OFF ^
    -DBUILD_PLUGIN_P2G=OFF ^
    -DBUILD_PLUGIN_PCL=OFF ^
    -DBUILD_PLUGIN_PGPOINTCLOUD=OFF ^
    -DBUILD_PLUGIN_SQLITE=OFF ^
    -DBUILD_PLUGIN_RIVLIB=OFF ^
    -DBUILD_PLUGIN_PYTHON=%PDAL_OPTIONAL_COMPONENTS% ^
    -DENABLE_CTEST=OFF ^
    -DWITH_APPS=ON ^
    -DCMAKE_VERBOSE_MAKEFILE=ON ^
    -DWITH_LAZPERF=ON ^
    -DLazperf_DIR=./laz-perf/install ^
    -DWITH_GEOTIFF=%PDAL_OPTIONAL_COMPONENTS% ^
    -DWITH_LASZIP=%PDAL_OPTIONAL_COMPONENTS% ^
    -DWITH_TESTS=ON ^
    -DNUMPY_INCLUDE_DIR=%OSGEO4W_ROOT%\apps\python27\lib\site-packages\numpy\core\include ^
	-DNUMPY_VERSION=1.8.1 ^
    -Dgtest_force_shared_crt=ON ^
    -DCMAKE_INSTALL_PREFIX=C:\pdalbin ^
    .
