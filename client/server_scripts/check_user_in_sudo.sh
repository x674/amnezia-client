CUR_USER=$(whoami 2> /dev/null || echo ~ | sed 's/.*\///');\
echo $LANG | grep -qE "en_US.UTF-8|^C.UTF-8" || export LC_ALL=C;\
[ "$CUR_USER" = "root" ] || groups $CUR_USER | grep -E "\<sudo\>|\<wheel\>" && sudo -nu $CUR_USER uname > /dev/null && sudo -n uname > /dev/null
