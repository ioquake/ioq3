pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh 'make release'
      }
    }
    stage('Deploy') {
      steps {
        sh 'COPYDIR=~/Desktop/jenkins/build make copyfiles & disown'
      }
    }
  }
  environment {
    ARCH = 'x86_64'
    PLATFORM = 'darwin'
  }
}