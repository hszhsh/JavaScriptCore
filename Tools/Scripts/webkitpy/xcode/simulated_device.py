# Copyright (C) 2014-2017 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import logging
import re
import subprocess

from webkitpy.xcode.device import Device
from webkitpy.xcode.simulator import Simulator
from webkitpy.common.host import Host

_log = logging.getLogger(__name__)


class SimulatedDevice(Device):
    """
    Represents a CoreSimulator device underneath a runtime
    """

    def __init__(self, name, udid, available, runtime, host):
        """
        :param name: The device name
        :type name: str
        :param udid: The device UDID (a UUID string)
        :type udid: str
        :param available: Whether the device is available for use.
        :type available: bool
        :param runtime: The iOS Simulator runtime that hosts this device
        :type runtime: Runtime
        :param host: The host which can run command line commands
        :type host: Host
        """
        super(SimulatedDevice, self).__init__(name, udid, host)
        self.available = available
        self.runtime = runtime

    @property
    def state(self):
        """
        :returns: The current state of the device.
        :rtype: Simulator.DeviceState
        """
        return Simulator.device_state(self.udid)

    @property
    def path(self):
        """
        :returns: The filesystem path that contains the simulator device's data.
        :rtype: str
        """
        return Simulator.device_directory(self.udid)

    @classmethod
    def create(cls, name, device_type, runtime):
        """
        Create a new CoreSimulator device.
        :param name: The name of the device.
        :type name: str
        :param device_type: The CoreSimulatort device type.
        :type device_type: DeviceType
        :param runtime:  The CoreSimualtor runtime.
        :type runtime: Runtime
        :return: The new device or raises a CalledProcessError if ``simctl create`` failed.
        :rtype: Device
        """
        device_udid = subprocess.check_output(['xcrun', 'simctl', 'create', name, device_type.identifier, runtime.identifier]).rstrip()
        _log.debug('"xcrun simctl create %s %s %s" returned %s', name, device_type.identifier, runtime.identifier, device_udid)
        Simulator.wait_until_device_is_in_state(device_udid, Simulator.DeviceState.SHUTDOWN)
        return Simulator().find_device_by_udid(device_udid)

    @classmethod
    def shutdown(cls, udid):
        """
        Shut down the given CoreSimulator device.
        :param udid: The udid of the device.
        :type udid: str
        """
        device_state = Simulator.device_state(udid)
        if device_state == Simulator.DeviceState.BOOTING or device_state == Simulator.DeviceState.BOOTED:
            _log.debug('xcrun simctl shutdown %s', udid)
            # Don't throw on error. Device shutdown seems to be racy with Simulator app killing.
            subprocess.call(['xcrun', 'simctl', 'shutdown', udid])

        Simulator.wait_until_device_is_in_state(udid, Simulator.DeviceState.SHUTDOWN)

    @classmethod
    def delete(cls, udid):
        """
        Delete the given CoreSimulator device.
        :param udid: The udid of the device.
        :type udid: str
        """
        SimulatedDevice.shutdown(udid)
        try:
            _log.debug('xcrun simctl delete %s', udid)
            subprocess.check_call(['xcrun', 'simctl', 'delete', udid])
        except subprocess.CalledProcessError:
            raise RuntimeError('"xcrun simctl delete" failed: device state is {}'.format(Simulator.device_state(udid)))

    @classmethod
    def reset(cls, udid):
        """
        Reset the given CoreSimulator device.
        :param udid: The udid of the device.
        :type udid: str
        """
        SimulatedDevice.shutdown(udid)
        try:
            _log.debug('xcrun simctl erase %s', udid)
            subprocess.check_call(['xcrun', 'simctl', 'erase', udid])
        except subprocess.CalledProcessError:
            raise RuntimeError('"xcrun simctl erase" failed: device state is {}'.format(Simulator.device_state(udid)))

    def install_app(self, app_path, env=None):
        # FIXME: This is a workaround for <rdar://problem/30273973>, Racey failure of simctl install.
        for x in xrange(3):
            if self._host.executive.run_command(['xcrun', 'simctl', 'install', self.udid, app_path], return_exit_code=True):
                return False
            try:
                bundle_id = self._host.executive.run_command([
                    '/usr/libexec/PlistBuddy',
                    '-c',
                    'Print CFBundleIdentifier',
                    self._host.filesystem.join(app_path, 'Info.plist'),
                ]).rstrip()
                self._host.executive.kill_process(self.launch_app(bundle_id, [], env=env, attempts=1))
                return True
            except RuntimeError:
                pass
        return False

    def launch_app(self, bundle_id, args, env=None, attempts=3):
        environment_to_use = {}
        SIMCTL_ENV_PREFIX = 'SIMCTL_CHILD_'
        for value in (env or {}):
            if not value.startswith(SIMCTL_ENV_PREFIX):
                environment_to_use[SIMCTL_ENV_PREFIX + value] = env[value]
            else:
                environment_to_use[value] = env[value]

        # FIXME: This is a workaround for <rdar://problem/30172453>.
        def _log_debug_error(error):
            _log.debug(error.message_with_output())

        output = None
        for x in xrange(attempts):
            output = self._host.executive.run_command(
                ['xcrun', 'simctl', 'launch', self.udid, bundle_id] + args,
                env=environment_to_use,
                error_handler=_log_debug_error,
            )
            match = re.match(r'(?P<bundle>[^:]+): (?P<pid>\d+)\n', output)
            if match:
                break

        if not match or match.group('bundle') != bundle_id:
            raise RuntimeError('Failed to find process id for {}: {}'.format(bundle_id, output))
        return int(match.group('pid'))

    def __repr__(self):
        return '<{device_info} State: {state}. Runtime: {runtime}, Available: {available}>'.format(
            device_info=super(SimulatedDevice, self).__repr__(),
            state=self.state,
            available=self.available,
            runtime=self.runtime.identifier)
