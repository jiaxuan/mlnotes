
1. modify ~/.profile

export TERM=xterm
export MANPATH=/home/zk8xmsb/local64/share/man:$MANPATH
export INTEL_HOME=/home/fxdev/apps/intel/c_studio_xe_2011_sp1_update2
export VTUNE_HOME=$INTEL_HOME/vtune_amplifier_xe

export JDK_HOME=/home/fxdev/apps64/jdk.1.6.0_29_64
export JAVA_HOME=$JDK_HOME
export CLASSPATH=$CLASSPATH:/home/zk8xmsb/.lein/self-installs/leiningen-2.3.4-standalone.jar
export APPS=/home/fxdev/apps64
export APPS2=/home/fxdev/apps64/gcc4.4.3_64-compliant
export APPS3=/home/fxdev/apps/gcc-4.4.3-x86_64-compliant
export PATH=$JDK_HOME/bin:$PATH
export PATH=$PATH:/sbin:/usr/sbin
export PATH=$APPS3/binutils-2.18/bin:$PATH
export PATH=$APPS3/compiler/gcc-4.4.3/bin:$PATH
export PATH=$APPS3/gdb-7.5/bin:$PATH
export PATH=$APPS3/sqsh-2.1.8/bin:$PATH
export PATH=$APPS3/python-2.7/bin:$PATH
export PATH=~/local64/bin:~/local/bin:$PATH
export LD_LIBRARY_PATH=/home/zk8xmsb/local64/openssl-1.0.1g/lib:$LD_LIBRARY_PATH


2. build git, don't run configure, use the following make directly

make prefix=/home/zk8xmsb/local64 CURLDIR=/home/zk8xmsb/local64 NO_R_TO_GCC_LINKER=YesPlease EXPATDIR=/home/fxdev/apps/gcc-4.4.3-x86_64-compliant/expat-2.1.0 OPENSSLDIR=/home/zk8xmsb/local64/openssl-1.0.1g ZLIB_PATH=/home/fxdev/apps/gcc-4.4.3-x86_64-compliant/zlib-1.2.3 install

3. setup proxy for git

git config --global http.proxy http://myusername:mypassword@proxy.acme.com:8080 


4. install YouCompleteMe

edit /home/zk8xmsb/.vim/bundle/YouCompleteMe/third_party/ycmd/build.sh to add the following

cmake_args+=" -DPYTHON_LIBRARY=/home/fxdev/apps/gcc-4.4.3-x86_64-compliant/python-2.7/lib/libpython2.7.so"
cmake_args+=" -DPYTHON_INCLUDE_DIR=/home/fxdev/apps/gcc-4.4.3-x86_64-compliant/python-2.7/include/python2.7"

before (near the end):

if [ -z "$YCM_TESTRUN" ]; then
  install $cmake_args $EXTRA_CMAKE_ARGS
else
  testrun $cmake_args $EXTRA_CMAKE_ARGS
fi


YouCompleteMe server failed to start when switching back to the old .profile. Worked properly
when using above .profile


