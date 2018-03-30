#!/bin/bash
# run from outside mwlwifi-8997

rm -rf mwlwifi-8997.tgz 

cd mwlwifi-8997
make clean; ./make.sh

rm -rf *.o *.mod.* modules.order Module.symvers
rm -rf .*.cmd .*.swp .git .tmp_versions
cd ..

tar cvfz mwlwifi-8997.tgz mwlwifi-8997/
ls -lh mwlwifi-8997.tgz
date
