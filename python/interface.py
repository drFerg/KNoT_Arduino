#Interface:
class SENSOR_TYPES(object):
	ALL = 'ALL'
	LIGHT = 'LIGHT'
	TEMP = 'TEMP'


class Device(object):
	def __init__(self, deviceAddr, name, devType, rate):
		self.deviceAddr = deviceAddr
		self.name = name
		self.type = devType
		self.rate = rate

class Response(object):
	"""docstring for Response"""
	def __init__(self, deviceAddr, data):
		super(Response, self).__init__()
		self.deviceAddr = deviceAddr
		self.data = data
		

def print_response(response):
	print "Received messages from:", response.deviceAddr
	print "Response data:", response.data


class KnotNetwork(object):
	"""docstring for KnotNetwork"""
	def __init__(self, name):
		self.name = name
		self.attachedDevices = {}
	# Queries network for devices of a type - blocks for 1 sec and returns results
	def queryNetwork(self, type=SENSOR_TYPES.ALL):
	    return [Device(1,"Bedroom Temp", SENSOR_TYPES.TEMP, 1), 
	    		Device(34, "Bedroom Light", SENSOR_TYPES.LIGHT, 5)]

	def registerSensor(self, sensorAddr, rate, callbackFunction):
		callbackFunction(Response(1, 20))
		return True

	def registerSensors(selfs):
	    return True

	def unregisterSensor(self, sensorAddr):
	    return True

	def pingDevice(deviceAddr):
		return True

	def deviceDetails(deviceAddr):
		return self.attachedDevices[deviceAddr]


if __name__ == "__main__":
	network = KnotNetwork("fred")

	print network.queryNetwork()

	network.registerSensor(1, 4, print_response)