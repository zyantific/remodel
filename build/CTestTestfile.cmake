# CMake generated Testfile for 
# Source directory: /Users/athre0z/Development/remodel
# Build directory: /Users/athre0z/Development/remodel/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(remodel-unittests "remodel_run_unittests")
subdirs(testing/gtest-1.7.0)
