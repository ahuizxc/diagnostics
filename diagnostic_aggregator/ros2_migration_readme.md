Overview

Aggregator is a node that subscribes to /diagnostics, processes it
and republishes aggregated data on /diagnostics_agg. The aggregator
creates a series of analyzers according to the specifications of its
private parameters. The aggregated diagnostics data is organized

Dependency 

	The diagnostic package has dependency on bond-core (ROS2 package) and some system libraries e.g. psutil
                1.1 To install bond-core package, copy the code from bond-core repository and build with diagnostic package.
                1.2 The psutil can be installed for python3 by executing command
                    # sudo -H python3 -m pip install psutil

Build proccedure and testing

	Note: If package is already build, start with testing (step #3) 
	1. Get pacakge at local system 

                1.1 # mkdir -p diagnostics/src

                1.2 # cd diagnostics/src

                1.3 # git clone git@github.com:kishornaik10/bond_core.git 
                    or git clone https://github.com/kishornaik10/bond_core.git
                    (checkout for ros2-devel branch)

                1.4 # git clone git@github.com:vaibhavbhadade/diagnostics.git
                    (checkout for ros2-devel branch)

                1.5 # source /opt/ros/crystal/setup.sh

	2. Build the package

                 2.1 # cd ../
                 2.2 # colcon build

	3. Do the test

		 3.1 # colcon test

	4. The executables are generated follow the below steps to run tests.

		4.1 source local setup

		 # source install/local_setup.sh

		4.2 run the executables.

		 All test cases can be run in following way.  we need to kill the nodes launch by test after test cases execution. This is bug in launch services .

			 1.  python3 src/diagnostics/diagnostic_aggregator/test/add_analyzers_test.py

			 2.  ps -ef // to find out node process id

			 3.  kill -9 "process id of newly started node for test case "

			 4.  python3 src/diagnostics/diagnostic_aggregator/test/aggregator_test.py

			 5.  ps -ef

			 6.  kill -9 "process id of newly started node for test case "

			 7.  python3 src/diagnostics/diagnostic_aggregator/test/expected_stale_test.py

			 8.  ps -ef

			 9.  kill -9 "process id of newly started node for test case "

			 10. python3 src/diagnostics/diagnostic_aggregator/test/multiple_match_test.py

			 11. ps -ef

			 12. kill -9 "process id of newly started node for test case "



ROS2 Migration changes 

	1. Basic design and concept of aggregator node are same as per ros,this
	node contain two analyzers GenericAnalyzer and AnalyzerGroup to hold 
	and categorise messsages. 

	2. Defination of base anlyzer class has been changed and extra parameter 
	added to pass paramters to analyzers 

	3. base analyzers will have following fuctionalities as per ros 

	    init() -  Loads parameters from yaml file

	    match() - Returns true/false if interested in viewing a status message

	    analyze() - Analyze a new message

	    report() - Report results or state

	    getPath() - Get complete path (anything prepended onto status names)

	    getName() - Get nice name, like "Motors" or "Sensors" 


	4. To configure the analyzer we give analyzer paramter in private namespace 
	'analyzers_params' as below :

			analyzers_params:
				  prefix1:
				    type: diagnostic_aggregator/GenericAnalyzer
				    path: First
				    remove_prefix: [ 'prefix1' ]
				    find_and_remove_prefix: [ 'find1_items']
				    startswith: [
				      'pref1a' ]
				    contains: [
				      'contains1a']
				    name: [
				      'name1' ]
				    expected: [
				      'prefix1: expected1a'] 


	5. Xmlrpc variables replaced by string types variables and required modification/changes incorporated. 

	6. Parametres for analyzers taken from yaml file only so analyzers created using paramter 
	service and corresponding code and logical parsing changes added to devired class of analyzers 

	7. Rest of the features of aggregator is same as previous.  

Limitations

        1. Publish rate is fixed 1hz.   	
	
        2. This is not designed to be a keepalive, it uses potentially unreliable transports and does not have tight timeouts, and there may be stale data due to aggregation.
	
	3. psutils should be install to run colcon test cleanly. 
