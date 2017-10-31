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
        sh 'COPYDIR=./artifacts make copyfiles & disown'
      }
    }
    stage('Clean') {
      steps {
        sh '''make distclean
'''
      }
    }
  }
  environment {
    ARCH = 'x86_64'
    PLATFORM = 'darwin'
  }
}
post {
        always {
            archive 'build/libs/**/*.jar'
            junit 'build/reports/**/*.xml'
        }
    }
}
