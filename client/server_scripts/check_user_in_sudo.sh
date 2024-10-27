echo $LC_MESSAGES | grep -qE "en_US.UTF-8|C.UTF-8" || export LC_MESSAGES=C.UTF-8;\
CUR_USER=$(whoami);\
groups $CUR_USER | grep sudo && sudo -n -u $CUR_USER sh -c "sudo -n uname 2>&1 > /dev/null"
