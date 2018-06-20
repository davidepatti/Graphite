#!/bin/bash
# author: soumyajit poddar

# Output prefix to use
oprefix="[COPYCAT]"



#################################################################
#                                                               #
#                           FUNCTIONS                           #
#                                                               #
#################################################################

# parsec_init
#
# Initialize the script
#
# Arguments: none
function parsec_init {
  # We need to hard-wire a few commands because we need them for the path detection
  PWD="pwd"
  BASENAME="basename"
  DIRNAME="dirname"

  # Try to find path
  uniquefile=".parsec_uniquefile"
  parsecdir=""
  if [ ! -z "${PARSECDIR}" ]; then
    # User defined PARSECDIR, check it
    parsecdir="${PARSECDIR}"
    if [ ! -f "${parsecdir}/${uniquefile}" ]; then
      echo "${oprefix} Error: Variable PARSECDIR points to '${PARSECDIR}', but this does not seem to be the PARSEC directory. Either unset PARSECDIR to make me try to autodetect the path or set it to the correct value."
      exit 1
    fi
  else
    # Try to autodetect path by looking at path used to invoke this script

    # Try to extract absoute or relative path
    if [ "${0:0:1}" == "/" ]; then
      # Absolute path given
      eval parsecdir=$(${DIRNAME} $(${DIRNAME} $0))
      # Check
      if [ -f "${parsecdir}/${uniquefile}" ]; then
        PARSECDIR=${parsecdir}
      fi
    else
      # No absolute path, maybe relative path?
      eval parsecdir=$(${PWD})/$(${DIRNAME} $(${DIRNAME} $0))
      # Check
      if [ -f "${parsecdir}/${uniquefile}" ]; then
        PARSECDIR=${parsecdir}
      fi
    fi

    # If PARSECDIR is still undefined, we try to guess the path
    if [ -z "${PARSECDIR}" ]; then
      # Check current directory
      if [ -f "./${uniquefile}" ]; then
        eval "parsecdir=\$(${PWD})"
        PARSECDIR=${parsecdir}
      fi
    fi
    if [ -z "${PARSECDIR}" ]; then
      # Check next-higher directory
      if [ -f "../${uniquefile}" ]; then
        eval "parsecdir=\$(${PWD})/.."
        PARSECDIR=${parsecdir}
      fi
    fi
  fi

  # Make sure PARSECDIR is defined and exported
  if [ -z "${PARSECDIR}" ]; then
    echo "${oprefix} Error: Unable to autodetect path to the PARSEC benchmark suite. Either define an environment variable PARSECDIR, or edit ${me} and set PARSECDIR to the correct value at the beginning of the file."
    exit 1
  fi
  export PARSECDIR

  # Eliminate trailing `/.' from PARSECDIR
  PARSECDIR=${PARSECDIR/%\/./}

  # Determine OS name to use for automatically determined PARSECPLAT
  case "${OSTYPE}" in
  *linux*)   ostype="linux";;
  *solaris*) ostype="solaris";;
  *bsd*)     ostype="bsd";;
  *aix*)     ostype="aix";;
  *hpux*)    ostype="hpux";;
  *irix*)    ostype="irix";;
  *amigaos*) ostype="amigaos";;
  *beos*)    ostype="beos";;
  *bsdi*)    ostype="bsdi";;
  *cygwin*)  ostype="windows";;
  *darwin*)  ostype="darwin";;
  *interix*) ostype="interix";;
  *os2*)     ostype="os2";;
  *osf*)     ostype="osf";;
  *sunos*)   ostype="sunos";;
  *sysv*)    ostype="sysv";;
  *sco*)     ostype="sco";;
  *)         ostype="${OSTYPE}";;
  esac

  # Determine HOST name to use for automatically determined PARSECPLAT
  case "${HOSTTYPE}" in
  *i386*)    hosttype="i386";;
  *x86_64*)  hosttype="amd64";;
  *amd64*)   hosttype="amd64";;
  *i486*)    hosttype="amd64";;
  *sparc*)   hosttype="sparc";;
  *sun*)     hosttype="sparc";;
  *ia64*)    hosttype="ia64";;
  *itanium*) hosttype="ia64";;
  *powerpc*) hosttype="powerpc";;
  *ppc*)     hosttype="powerpc";;
  *alpha*)   hosttype="alpha";;
  *mips*)    hosttype="mips";;
  *arm*)     hosttype="arm";;
  *)         hosttype="${HOSTTYPE}";;
  esac

  # Determine first part of value to use for PARSECPLAT environment variable if not defined by user
  # Note: We will append the compiler configuration to that to get the final value for PARSECPLAT
  hostostype="${hosttype}-${ostype}"

  # Define some global directories
  benchdir=${parsecdir}/pkgs
  logdir=${parsecdir}/log

  # Source global configuration file with alias definitions, package dependencies etc.
  #parsecconfig="${PARSECDIR}/config/parsec.conf"
  #if [ -f "${parsecconfig}" ]; then
  #  source ${parsecconfig}
  #else
  #  echo "${oprefix} Error: Cannot load global configuration file '${parsecconfig}'."
  #  exit 1
  #fi

  # Try to load OS-specific configuration to get binaries and correct arguments
  sysconfig="${PARSECDIR}/config/${ostype}.sysconf"
  if [ -f "${sysconfig}" ]; then
    source ${sysconfig}
  else
    echo "${oprefix} Error: Cannot load system configuration file '${sysconfig}' for OS type '${ostype}'. Please create a new system configuration file."
    exit 1
  fi

  # Setup environment so PARSEC tools are usable by other programs
  if [ -z "${PATH}" ]; then
    export PATH="${PARSECDIR}/bin"
  else
    export PATH="${PARSECDIR}/bin:${PATH}"
  fi
}


#################################################################
#                                                               #
#                             MAIN                              #
#                                                               #
#################################################################

# Check version
if [ ${BASH_VERSINFO[0]} -lt 3 ]; then
  # We need certain Bash 3 features. e.g. PIPESTATUS (which was available but broken in earlier versions)
  echo "${oprefix} Warning: At least bash version 3 is recommended. Earlier versions might not function properly. Current version is $BASH_VERSION."
fi

# Execute functions to setup environment
parsec_init

me="copycat.sh"
# Check options and take options
usage="\
Usage: $me [OPTION]

Options:
    tools
    apps
    libs
Examples:
        $me tools
        $me apps
        $me libs"


parse_mode=""
config_file="graphite.bldconf"

if [ -z "$1" ]; then
  echo "${oprefix} Error: No arguments provided!"
  echo "${usage}"
  exit 1
else
  parse_mode="$1"
  if [ "$1" == "tools" ]; then
    echo "${oprefix} Now using tools option!"
    cd config
    if ! [ -a "${config_file}" ]; then
      echo "${oprefix} Error: ${config_file} doesn't exist"
      exit 1
    fi
    cd ..
    for f in yasm cmake libtool
    do
      cp -f config/${config_file} pkgs/tools/$f/parsec/
      cd pkgs/tools/$f/parsec
      sed '/source/ i\source ${PARSECDIR}/pkgs/tools/'"${f}"'/parsec/gcc.bldconf' ${config_file} > tmpfile
      mv tmpfile ${config_file}
      cd ../../../..
    done
  elif [ "$1" == "apps" ]; then
    echo "${oprefix} Now using apps option!"
    cd config
    if ! [ -a "${config_file}" ]; then
      echo "${oprefix} Error: ${config_file} doesn't exist"
      exit 1
    fi
    cd ..
    for f in blackscholes bodytrack facesim ferret fluidanimate freqmine raytrace swaptions vips x264
    do
      cp -f config/${config_file} pkgs/apps/$f/parsec/
      cd pkgs/apps/$f/parsec
      sed '/source/ i\source ${PARSECDIR}/pkgs/apps/'"${f}"'/parsec/gcc-hooks.bldconf' ${config_file} > tmpfile
      mv tmpfile ${config_file}
      cd ../../../..
    done
  elif [ "$1" == "libs" ]; then
    echo "${oprefix} Now using libs option!"
    cd config
    if ! [ -a "${config_file}" ]; then
      echo "${oprefix} Error: ${config_file} doesn't exist"
      exit 1
    fi
    cd ..
    for f in glib  gsl  hooks  libjpeg  libxml2  mesa  parmacs  ssl  tbblib  uptcpip  zlib
    do
      cp -f config/${config_file} pkgs/libs/$f/parsec/
      cd pkgs/libs/$f/parsec
      sed '/source/ i\source ${PARSECDIR}/pkgs/libs/'"${f}"'/parsec/gcc-hooks.bldconf' ${config_file} > tmpfile
      mv tmpfile ${config_file}
      cd ../../../..
    done
  else
    echo "${oprefix} Error: Unknown option!"
    exit 1
  fi
fi
echo "${oprefix} Done"
exit 0
