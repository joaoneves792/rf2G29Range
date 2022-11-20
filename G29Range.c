//#include "PluginObjects.h"
//#include "InternalsPlugin.h"

#include <windef.h>


int g_wheelAngle = 0;

void WINAPI ProxyEnterRealtime(){
  g_wheelAngle = 0;
}

void WINAPI ExitRealtime(){
  g_wheelAngle = 0;
}

void WINAPI UpdateTelemetry(void *info){
  if(!g_wheelAngle){
    
    g_wheelAngle = 1;
  }

}
