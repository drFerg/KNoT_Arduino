from django.db import models

# Create your models here.
class Thing(models.Model):
    name = models.CharField(max_length=200, blank=False, primary_key=True)
    addr = models.IntegerField(blank=False)
    dev_type = models.IntegerField(blank=False)
    connected = models.BooleanField(default=False)
    data = models.IntegerField()