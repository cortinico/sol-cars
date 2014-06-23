#!/bin/bash

i=0
k=0
while [ 1 ] ;

do
  for k in $( seq 1 $1 )
  	do
  	echo "Eseguo $i "
  	./stressclient.sh $i  & 
  	i=$(( $i+1 ))
  	done 
	sleep $2
done                      

exit 0
