#!/usr/bin/env python3.6
#
# Software License Agreement (BSD License)
#
# Copyright (c) 2017, TNO IVS, Helmond, Netherlands
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of the TNO IVS nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# \author Rein Appeldoorn
from sys import path
print(path)
import rclpy
import diagnostic_analysis 
from diagnostic_msgs.msg import DiagnosticStatus
from diagnostic_updater import DiagnosticTask, Updater

from rclpy.parameter import Parameter
from time import sleep
import psutil
import socket


class CpuTask(DiagnosticTask):
    def __init__(self, warning_percentage):
        DiagnosticTask.__init__(self, "CPU Information")
        self._warning_percentage = int(warning_percentage)

    def run(self, stat):
        cpu_percentages = psutil.cpu_percent(percpu=True)
        cpu_average = sum(cpu_percentages) / len(cpu_percentages)

        stat.add("CPU Load Average", cpu_average)

        warn = False
        for idx, val in enumerate(cpu_percentages):
            stat.add("CPU {} Load".format(idx), "{}".format(val))
            if val > self._warning_percentage:
                warn = True

        if warn:
            stat.summary(DiagnosticStatus.WARN, "At least one CPU exceeds %d percent" % self._warning_percentage)
        else:
            stat.summary(DiagnosticStatus.OK, "CPU Average %.1f percent" % cpu_average)

        return stat


def main():
    hostname = socket.gethostname()
    #print('cpu_monitor_%s' % hostname.replace("-", "_"))
    #rospy.init_node('cpu_monitor_%s' % hostname.replace("-", "_"))
    rclpy.init()
    node = rclpy.create_node('cpu_monitor_%s' % hostname.replace("-", "_"))
    p=Parameter('~warning_percentage', Parameter.Type.INTEGER, 90)
    print(node.get_node_names())

    updater = Updater(node)
    updater.setHardwareID(hostname)
    #updater.add(CpuTask(rospy.get_param("~warning_percentage", 90)))
    updater.add(CpuTask(90))

    #rate = rospy.Rate(rospy.get_param("~rate", 1))
    while rclpy.ok():
        #rate.sleep()
        sleep(1)
        updater.update()


if __name__ == '__main__':
    main()