for task in 1 2 3
do
	qsub -S /bin/bash kmeans.sh -o taskReport$task.txt -e taskLog$task.txt -l nodes=1:ppn=12 -v task=$task
done
