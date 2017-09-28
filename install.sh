#!/bin/bash

vercomp () {
    if [[ $1 == $2 ]]
    then
        return 0
    fi
    local IFS=.
    local i ver1=($1) ver2=($2)
    # fill empty fields in ver1 with zeros
    for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
    do
        ver1[i]=0
    done
    for ((i=${#ver2[@]}; i<${#ver1[@]}; i++))
    do
        ver2[i]=0
    done
    for ((i=0; i<${#ver1[@]}; i++))
    do
        if [[ -z ${ver2[i]} ]]
        then
            # fill empty fields in ver2 with zeros
            ver2[i]=0
        fi
        if ((10#${ver1[i]} > 10#${ver2[i]}))
        then
            return 1
        fi
        if ((10#${ver1[i]} < 10#${ver2[i]}))
        then
            return 2
        fi
    done
    return 0
}

info (){
  echo -e "\033[0;32m-> \033[0m\033[0;01m $1\033[0;0m"
}

cleanup(){
  info "Cleaning up..."

  filename="scopion_${VERSION_TO_INSTALL}-${TARGET_PLATFORM}_${TARGET_ARCH}"

  if [ -f "${filename}.zip" ]; then
    execmd "rm ${filename}.zip"
  fi

  if [ -d ${filename} ]; then
    execmd "rm -r ${filename}"
  fi
}

cleanup_error(){
  cleanup

  case $TARGET_PLATFORM in
    Linux)
      if [ -f /etc/apt/sources.list.bak ]; then
        execmdsu "mv /etc/apt/sources.list.bak /etc/apt/sources.list"
      fi
      ;;
    Darwin)
      ;;
  esac
}

exit_fail(){
  if [ $1 = -1 ];then
    cleanup_error;
  fi
  \exit $1
}

execmd (){
  echo -e "\033[0;0m> $1\033[0;0m"
  if [ ! "$FLAG_D" = "1" ]; then
    eval $1
  fi
  if [ ! "$?" = "0" ]; then
    error "Failed with return code $?. Abort."
    exit_fail -1
  fi
}

execmdsu(){
  if [[ $EUID -ne 0 ]]; then
    #non-root
    execmd "sudo bash -c '$1'"
  else
    execmd "$1"
  fi
}

error (){
  echo -e "\033[0;31m[ERROR] \033[0m\033[0;01m $1\033[0;0m"
}

confirm (){
  if [ $# = 0 ]; then
    MSG="Ready to install. Continue?"
  else
    MSG=$1
  fi
  while true; do
    printf '%s ' "$MSG"
    printf "[y/n] > "
    read input
    if [ "$input" = "y" ]; then
      return 0;
    elif [ "$input" = "n" ]; then
      return 1;
    fi
  done
}

findscopc (){
  \type scopc &> /dev/null
  return $?
}

pre_install(){
  info "Preparing to install..."

  case $TARGET_PLATFORM in
    Linux)
      execmdsu "apt-get update"
      execmdsu "apt-get install -y wget ca-certificates gnupg unzip"
      execmdsu "cp /etc/apt/sources.list /etc/apt/sources.list.bak"
      execmdsu "echo \"deb http://apt.llvm.org/${TARGET_VERSION_NAME}/ llvm-toolchain-${TARGET_VERSION_NAME}-${LLVM_VERSION} main\" >> /etc/apt/sources.list"
      execmdsu "wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -"
      ;;
    Darwin)
      ;;
  esac
}

install_depends() {
  info "Installing dependencies..."

  case $TARGET_PLATFORM in
    Linux)
      execmdsu "apt-get update"
      execmdsu "apt-get -y --allow-unauthenticated install clang-${LLVM_VERSION} llvm-${LLVM_VERSION} libgc-dev exuberant-ctags"
      execmdsu "update-alternatives --install /usr/local/bin/clang clang `which clang-${LLVM_VERSION}` 10"
      execmdsu "update-alternatives --install /usr/local/bin/llc llc `which llc-${LLVM_VERSION}` 10"
      ;;
    Darwin)
      execmd "brew install llvm bdw-gc ctags"
      ;;
    esac
}

install(){
  filename="scopion_${VERSION_TO_INSTALL}-${TARGET_PLATFORM}_${TARGET_ARCH}"

  info "Downloading a binary..."
  execmd "wget https://github.com/coord-e/scopion/releases/download/v${VERSION_TO_INSTALL}/${filename}.zip"

  info "Installing..."
  execmd "unzip ${filename}.zip"
  execmdsu "cp -r ${filename}/* ${PREFIX}/"
}

post_install(){
  info "Final processes..."

  case $TARGET_PLATFORM in
    Linux)
      execmdsu "ldconfig"
      ;;
    Darwin)
      execmdsu "update_dyld_shared_cache"
      ;;
  esac

  execmd "hash -r"
}

usage() {
        echo "Usage: $0 [-d]"
        echo -e "\t-d: Dry run"
}

VERSION_TO_INSTALL="0.0.2"
PREFIX="/usr/local"

TARGET_VERSION="Unknown"
TARGET_VERSION_NAME="Unknown"
TARGET_OS="Unknown"

LLVM_VERSION="4.0"

UBUNTU_LEAST="16.04"
DEBIAN_LEAST="8"
MAC_LEAST="10.12"

while getopts dp:h OPT
do
    case $OPT in
        d)  FLAG_D=1
            ;;
        h)  usage
            exit
            ;;
    esac
done

shift $((OPTIND - 1))

findscopc && error "It seems that scopion is already installed. Try \`scopc -v\`. Abort." && exit_fail -1;
info "scopc isn't installed"

TARGET_ARCH=$(uname -m)
if [ ! $TARGET_ARCH = "x86_64" ]; then
  error "Archtecture $(uname -m) isn't supported. Abort."
  exit_fail -1;
fi

TARGET_PLATFORM=$(uname -s)
case ${TARGET_PLATFORM} in
  Linux)
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        TARGET_OS=$NAME
        TARGET_VERSION=$VERSION_ID
        if [ -z "$VERSION_CODENAME" ]; then
          TARGET_VERSION_NAME=$(echo $VERSION | sed -n 's/^[^(]*(\(.*\))$/\1/p') #debian? "8 (jessie)"
        else
          TARGET_VERSION_NAME=$(echo $VERSION_CODENAME | cut -d ' ' -f1 | tr -d '\n') #ubuntu?
        fi
        TARGET_VERSION_NAME=${TARGET_VERSION_NAME,,} #to lower
    else
      error "/etc/os-release not found. Abort."
      exit_fail -1
    fi

    case $TARGET_OS in
      *Ubuntu*)
        vercomp $TARGET_VERSION $UBUNTU_LEAST
        if [ $? = 2 ]; then
          error "Unsupported Version Ubuntu ${TARGET_VERSION}. Abort."
          exit_fail -1
        fi

        ;;
      *Debian*)
        vercomp $TARGET_VERSION $DEBIAN_LEAST
        if [ $? = 2 ]; then
          error "Unsupported Version Debian ${TARGET_VERSION}. Abort."
          exit_fail -1
        fi

        ;;
      *)
        error "Unsupported OS ${TARGET_OS}. Abort."
        exit_fail -1
        ;;
      esac
    ;;

  Darwin)
    TARGET_OS="macOS"
    TARGET_VERSION=$(sw_vers -productVersion)
    TARGET_VERSION_NAME=$(curl -s http://support-sp.apple.com/sp/product?edid=$TARGET_VERSION | sed -n 's/.*<configCode>\(.*\)<\/configCode>.*/\1/p')
    vercomp $TARGET_VERSION $MAC_LEAST
    if [ $? = 2 ]; then
      error "Unsupported Version $TARGET_VERSION. Abort."
      exit_fail -1;
    fi
    ;;

  *)
    error "Unsupported platform ${TARGET_PLATFORM}. Abort."
    exit_fail -1;;
esac

echo -n "Install prefix [${PREFIX}] > "
read PREFIX_R
if [ ! -z "$PREFIX_R" ]; then
  PREFIX=$PREFIX_R
fi

info "Platform: ${TARGET_PLATFORM}"
info "OS: ${TARGET_OS}"
info "Version: ${TARGET_VERSION_NAME} ${TARGET_VERSION}"
info "Prefix: ${PREFIX}"

confirm || exit_fail -1;

pre_install
install_depends
install
post_install

info "Final check..."
findscopc || error "Couldn't find scopc installed." || exit_fail -1;
execmd "scopc -V"

info "Installed. Enjoy!"

cleanup
