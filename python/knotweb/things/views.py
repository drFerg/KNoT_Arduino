from django.shortcuts import render
from models import Thing
# Create your views here.

def showThings(request):
	return render(request, 'things/things.html', {"things":Thing.objects.all()})