#!/bin/bash

DATA_GEN=/home/kashin/src/cpp/hw2/k-means/data-gen
KMEANS=/home/kashin/src/cpp/hw2/k-means/kmeans
REPORT_FILE=/home/kashin/src/cpp/hw2/k-means/report.txt
OPTIMAL_THREADS_NUMBER=12
DIMENSION=5

POINTS_NUMBERS=(100 500 1000 2500 5000 7500 10000 50000)
CLUSTERS_NUMBERS=(2 5 7 10 20 35 50)

if [ -f $REPORT_FILE ]; then
	rm $REPORT_FILE
fi

function measureFixedClustersNumberTime {
	CLUSTERS_NUMBER=50
	echo POINTS_NUMBER, CLUSTERS_NUMBER, THREADS_NUMBER, TIME
	for THREADS_NUMBER in 1 $OPTIMAL_THREADS_NUMBER
	do
		export OMP_NUM_THREADS=$THREADS_NUMBER
		for POINTS_NUMBER in "${POINTS_NUMBERS[@]}"
		do
			$DATA_GEN $DIMENSION $POINTS_NUMBER $CLUSTERS_NUMBER data.txt
			echo -n $POINTS_NUMBER, $CLUSTERS_NUMBER, $THREADS_NUMBER,' '
			exec 3>&1 4>&2
			elapsed_time=$( /usr/bin/time -f "%e" sh -c "$KMEANS $CLUSTERS_NUMBER data.txt clusters.txt 1>&3 2>&4;" 2>&1 )  # Captures time only.
			exec 3>&- 4>&-
			echo $elapsed_time
		done
	done
}

function measureFixedPointsNumberTime {
	POINTS_NUMBER=50000
	echo POINTS_NUMBER, CLUSTERS_NUMBER, THREADS_NUMBER, TIME
	for THREADS_NUMBER in 1 $OPTIMAL_THREADS_NUMBER
	do
		export OMP_NUM_THREADS=$THREADS_NUMBER
		for CLUSTERS_NUMBER in "${CLUSTERS_NUMBERS[@]}"
		do
			$DATA_GEN $DIMENSION $POINTS_NUMBER $CLUSTERS_NUMBER data.txt
			echo -n $POINTS_NUMBER, $CLUSTERS_NUMBER, $THREADS_NUMBER,' '
			exec 3>&1 4>&2
			elapsed_time=$( /usr/bin/time -f "%e" sh -c "$KMEANS $CLUSTERS_NUMBER data.txt clusters.txt 1>&3 2>&4;" 2>&1 )  # Captures time only.
			exec 3>&- 4>&-
			echo $elapsed_time
		done
	done
}

function measureThreadsNumberTime {
	$DATA_GEN 5 50000 50 data.txt
	echo THREADS_NUMBER, TIME
	for ((THREADS_NUMBER = 1; THREADS_NUMBER <= 24; THREADS_NUMBER++)) 
	do
		export OMP_NUM_THREADS=$THREADS_NUMBER
		echo -n $THREADS_NUMBER,' '
		exec 3>&1 4>&2
		elapsed_time=$( /usr/bin/time -f "%e" sh -c "$KMEANS 50 data.txt clusters.txt 1>&3 2>&4;" 2>&1 )  # Captures time only.
		exec 3>&- 4>&-
		echo $elapsed_time
	done
}

# measureThreadsNumberTime > $REPORT_FILE
#measureThreadsNumberTime 1> $REPORT_FILE
TASK=$1
case $TASK in
	1) measureThreadsNumberTime
		;;
	2) measureFixedPointsNumberTime
		;;
	3) measureFixedClustersNumberTime
		;;
esac

#R -q -f plotReport.R
# evince report.pdf&
