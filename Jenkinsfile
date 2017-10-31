pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'make release'
      }
    }
    stage('Archive') {
      steps {
        sh '''COPYDIR=~/Desktop/_jenkins/ make copyfiles
cd ~/Desktop/ && zip build$RANDOM.zip _jenkins'''
      }
    }
  }
  environment {
    ARCH = 'x86_64'
    PLATFORM = 'darwin'
  }
}