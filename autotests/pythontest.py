#!/usr/bin/env python
#-*- coding: utf-8 -*-

import sys

sys.path.append(sys.argv[1])

from PyQt5 import QtWidgets

from PyKF5 import KJobWidgets

def main():
    app = QtWidgets.QApplication(sys.argv)

    jt = KJobWidgets.KStatusBarJobTracker()
    jt.setStatusBarMode(KJobWidgets.KStatusBarJobTracker.NoInformation)

if __name__ == '__main__':
    sys.exit(main())
