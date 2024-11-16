CUR_USER=$(whoami 2> /dev/null || echo ~ | sed 's/.*\///');\
groups $CUR_USER;\
echo $LANG | grep -qE "en_US.UTF-8|C.UTF-8" || export LC_ALL=C;\
if [ "$CUR_USER" = "root" ] && command -v sudo > /dev/null 2>&1; then sudo -nu $CUR_USER sudo -n uname > /dev/null;\
else [ "$CUR_USER" != "root" ] && groups $CUR_USER | grep -qE " sudo| wheel" && sudo -nu $CUR_USER sudo -n uname > /dev/null; fi
