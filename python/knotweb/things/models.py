from django.db import models
from datetime import datetime

# Create your models here.
class Thing(models.Model):
    name = models.CharField(max_length=200, blank=False, primary_key=True)
    addr = models.IntegerField(blank=False)
    dev_type = models.IntegerField(blank=False)
    connected = models.IntegerField(default=0)
    refreshed = models.DateTimeField(default=datetime.now())
    data = models.IntegerField()

class Queried(models.Model):
    name = models.CharField(max_length=200, blank=False, primary_key=True)
    addr = models.IntegerField(blank=False)
    dev_type = models.IntegerField(blank=False)
    refreshed = models.DateTimeField(default=datetime.now())

