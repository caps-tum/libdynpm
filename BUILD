To create an RPM from a checked out repository the following steps are required:

# First ensure pmix-devel and libevent-devel are installed

# change into the checked out repository
cd <your path to libdynpm>
# Generate autoconf files
./autogen.sh
# configure the repository (this will search for PMIx and libevent-devel
# via pkg-config
./configure
# create a tar-ball including all sources and the spec-file
make dist
# build the actual RPM and the SRPM
rpmbuild -ta --clean libdynpm-0.1.tar.gz

The actual RPM and SRPM can be found in ~/rpmbuild/RPMS/<arch> and
~rpmbuild/SRPMS respectively