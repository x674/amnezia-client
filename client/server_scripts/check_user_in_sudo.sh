CUR_USER=$(whoami 2> /dev/null || echo ~ | sed 's/.*\///');\
echo $LANG | grep -qE "en_US.UTF-8|^C.UTF-8" || export LC_ALL=C;\
if [ "$CUR_USER" = "root" ]; then command -v sudo > /dev/null 2>&1 && sudo -nu $CUR_USER sudo -n uname > /dev/null;\
else groups $CUR_USER | grep -E "\<sudo\>|\<wheel\>" && sudo -nu $CUR_USER sudo -n uname > /dev/null; fi
