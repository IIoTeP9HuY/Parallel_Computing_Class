report = read.csv("report.txt")
pdf("report.pdf")
plot(report)
dev.off()

byThreadNumber <- function() {
  report = read.csv("taskReport1_12.csv")
  pdf("byThreadNumber.pdf")
  plot(report)
  report$TIME = report$TIME[1] / report$TIME
  plot(report, main="Speedup")
  
  report$TIME = report$TIME / seq(1:24)
  plot(report, main="Effectiveness")
  dev.off()
}

withFixedClusters <- function() {
  report = read.csv("taskReport3_12.csv")
  pdf("withFixedClusters.pdf")
  clustersNumber = report["CLUSTERS_NUMBER"]
  clustersNumber = clustersNumber[1, 1]
  threadsNumbers = unique(report["THREADS_NUMBER"])
  report = report[report["CLUSTERS_NUMBER"] == clustersNumber, -2];
  thread1report = report[report["THREADS_NUMBER"] == 1, -2];
  plot(thread1report, col = "blue", main="1 and 12 threads") 
  par(new=TRUE)
  thread12report = report[report["THREADS_NUMBER"] == 12, -2];
  points(thread12report, col = "red") 
  
  pointsNumbers = thread1report$POINTS_NUMBER
  boost = thread1report$TIME / thread12report$TIME
  print(pointsNumbers)
  print(boost)
  plot(pointsNumbers, boost, main="Speedup")
  
  effectiveness = boost / 12;
  plot(pointsNumbers, effectiveness, main="Effectiveness")
  dev.off()
}

withFixedPoints <- function() {
  report = read.csv("taskReport2_12.csv")
  pdf("withFixedPoints.pdf")
  pointsNumber = report["POINTS_NUMBER"]
  pointsNumber = pointsNumber[1, 1]
  threadsNumbers = unique(report["THREADS_NUMBER"])
  report = report[report["POINTS_NUMBER"] == pointsNumber, -1];
  thread1report = report[report["THREADS_NUMBER"] == 1, -2];
  plot(thread1report, col = "blue", main="1 and 12 threads") 
  par(new=TRUE)
  thread12report = report[report["THREADS_NUMBER"] == 12, -2];
  points(thread12report, col = "red") 
  
  clustersNumbers = thread1report$CLUSTERS_NUMBER
  boost = thread1report$TIME / thread12report$TIME
  print(clustersNumbers)
  print(boost)
  plot(clustersNumbers, boost, main="Speedup")
  
  effectiveness = boost / 12;
  plot(clustersNumbers, effectiveness, main="Effectiveness")
  dev.off()
}

# clustersNumbers = unique(report["CLUSTERS_NUMBER"])

# pdf("report.pdf")
# for (clustersNumber in clustersNumbers[,]) {
#   plot(report[report["CLUSTERS_NUMBER"] == clustersNumber, -2], main=clustersNumber)  
# }
# dev.off()
