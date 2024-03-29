from labscript import *

from labscript_devices.PulseBlasterUSB import PulseBlasterUSB
from labscript_devices.Eurocard_Synth.labscript_devices import *

# For dummy pseudoclock
# from labscript_devices.DummyPseudoclock.labscript_devices import DummyPseudoclock
# from labscript_devices.DummyIntermediateDevice import DummyIntermediateDevice

#############################################################
#   CONNECTION TABLE
#############################################################

# Use a virtual, or 'dummy', device for the psuedoclock
# DummyPseudoclock(name='pulseblaster_0')

#############################################################
#   PULSEBLASTER
#############################################################

PulseBlasterUSB(name='pulseblaster_0', board_number=0, time_based_stop_workaround=True, time_based_stop_workaround_extra_time=0.5)

ClockLine(name='pulseblaster_E_clockline0', pseudoclock=pulseblaster_0.pseudoclock,connection='flag 1')
ClockLine(name='pulseblaster_E_clockline1', pseudoclock=pulseblaster_0.pseudoclock,connection='flag 7')


Eurocard_Synth(name='E_controller0',com_port='COM5',channel=0,parent_device=pulseblaster_E_clockline0)
Eurocard_DDS(name='E_DDS0', parent_device=E_controller0, connection='DDS_channel0')
Eurocard_Synth(name='E_controller1',com_port='COM5',channel=1,parent_device=pulseblaster_E_clockline1)
Eurocard_DDS(name='E_DDS1', parent_device=E_controller1, connection='DDS_channel1')

if __name__ == '__main__':
    start()
    stop(1)

