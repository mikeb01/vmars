#!/bin/bash

#msg=$'8=FIX.4.4\0019=866\00135=W\00149=LMXBLM\00156=skyonfx\00134=359559\00152=20130704-23:54:22.000\001262=4005,10000\00148=4005\00122=8\001268=29\001269=0\001270=150.754\001271=50\001272=20130704\001273=23:54:21.999\001269=0\001270=150.746\001271=100\001269=0\001270=150.745\001271=100\001269=0\001270=150.74\001271=250\001269=0\001270=150.737\001271=100\001269=0\001270=150.736\001271=100\001269=0\001270=150.732\001271=300\001269=0\001270=150.721\001271=50\001269=0\001270=150.716\001271=250\001269=0\001270=150.714\001271=500\001269=0\001270=150.706\001271=200\001269=0\001270=145.394\001271=0.1\001269=1\001270=150.776\001271=50\001269=1\001270=150.779\001271=200\001269=1\001270=150.785\001271=100\001269=1\001270=150.788\001271=100\001269=1\001270=150.79\001271=250\001269=1\001270=150.798\001271=300\001269=1\001270=150.801\001271=50\001269=1\001270=150.803\001271=50\001269=1\001270=150.804\001271=100\001269=1\001270=150.808\001271=50\001269=1\001270=150.815\001271=50\001269=1\001270=150.816\001271=100\001269=1\001270=150.824\001271=200\001269=1\001270=150.827\001271=400\001269=1\001270=150.845\001271=50\001269=1\001270=154.602\001271=1\001269=1\001270=154.642\001271=1\00110=028\001'
msg1=$'8=FIX.4.2\0019=130\00135=D\00134=659\00149=BROKER04\00156=REUTERS\00152=20070123-19:09:43\00138=1000\00159=1\001100=N\00140=1\00111=ORD10001\00160=20070123-19:01:17\00155=HPQ\00154=1\00121=2\00110=004\001'
msg2=$'8=FIX.4.2\0019=224\00135=8\00149=REUTERS\00156=BROKER04\00134=4\00152=20170412-23:44:20.150\00160=20170412-23:44:20.117\00120=0\00122=8\0016=0\00111=ORD10001\00117=QLAKAAAAAAAAAAAC\00148=180385\00155=HPQ\0011=1\00137=QLAKAAAAAAAAAAAB\001151=10.0\00114=0.0\00138=10.0\00154=1\00144=100.0\00159=0\001150=0\00139=0\00110=010\001'

printf $msg1 | nc -N $1 9999
printf $msg2 | nc -N $1 9999
