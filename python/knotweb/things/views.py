import serial, sys
from time import gmtime, strftime
import serial_wrapper as sw

from django.shortcuts import render
from models import Thing, Queried
# Create your views here.



def showThings(request):
	return render(request, 'things/things.html', {"things":Thing.objects.all()})


def query(request):
    return render(request, 'things/query.html', {"things":Queried.objects.all()})

def queryDevices(request):
    Queried.objects.all().delete()
    gateway = serial.Serial("/dev/ttyUSB0", "38400", timeout=5)
    gateway.write(sw.build_query(1))
    gateway.close()
    return render(request, 'things/query.html', {"things":Queried.objects.all()})

def connect(request):
    gateway = serial.Serial("/dev/ttyUSB0", "38400", timeout=5)
    gateway.write(sw.build_connect(int(request.GET.get("addr")), 5))
    gateway.close()
    return render(request, 'things/things.html', {"things":Thing.objects.all()})