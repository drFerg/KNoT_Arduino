from django.conf.urls import patterns, include, url

from django.contrib import admin
from things import views
from django.conf import settings
from django.conf.urls.static import static
admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'knotweb.views.home', name='home'),
    # url(r'^blog/', include('blog.urls')),

    url(r'^admin/', include(admin.site.urls)),
    url(r'^$', 'things.views.showThings', name='index'),
    url(r'^query/', 'things.views.query'),
    url(r'^queryDevices/', 'things.views.queryDevices'),
    url(r'^connect/$', 'things.views.connect'),
) + static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)
