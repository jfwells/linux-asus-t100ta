#!/bin/bash

TODAY=`date +"%Y%m%d"`

cd ..
tar cvf atomisp_testapp_$TODAY.tar atomisp_testapp \
    --exclude='.*' \
    --exclude='archive' \
    --exclude='*~'

cp atomisp_testapp_$TODAY.tar /var/www/html/downloads/intel/isp/atomisp_testapp/

exit 0

