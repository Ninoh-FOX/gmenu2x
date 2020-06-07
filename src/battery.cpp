#include "battery.h"

#include "surfacecollection.h"

#include <SDL.h>
#include <cstdio>
#include <sstream>

int lastChecking=SDL_GetTicks();
int batt_average[10]={0,0,0,0,0,0,0,0,0,0};   // Array con los últimos 10 valores leídos de la batería
int battavg_idx=0;                            // Índice al valor que toca actualizar en la próxima lectura

/**
 * Reads the current battery state and returns a number representing its level
 * of charge.
 * @return A number representing battery charge: 0 means fully discharged,
 * 5 means fully charged, 6 represents running on external power.
 */
// la función de lectura de batería se divide en 2, una para ver carga y otra para ver si está conectada por usb
unsigned short getBatteryLevel()
{
  int battval=0;

  // si han pasado más de 2 minutos sin leer datos, marca la lectura a 0 para actualizar todos los valores
  if((SDL_GetTicks()-lastChecking)>120000)
    batt_average[battavg_idx]=0;

  // si la lectura está a 0 o han pasado al menos 5 segundos, hace una nueva lectura
  if(batt_average[battavg_idx]==0 || (SDL_GetTicks()-lastChecking)>5000) {
    lastChecking=SDL_GetTicks();

    FILE *batteryHandle = NULL;
#if defined(PLATFORM_A320) || defined(PLATFORM_GCW0) || defined(PLATFORM_NANONOTE)
    batteryHandle = fopen("/sys/class/power_supply/battery/voltage_now", "r");
#endif
    if (batteryHandle) {
		#ifdef BATTERY_RG350
		/* voltaje maximo de la RG es 4320000 */
		#define MAX_VOLTAGE 4200000
		#define MIN_VOLTAGE 3300000
		/* voltaje maximo de la RG es 4385000 con el cable USB */
		#define USB_VOLTAGE 65000
		#ifdef BATTERY_PG2
		/* voltaje maximo de la PG2 es 4220000 */
		#define MAX_VOLTAGE 4150000
		#define MIN_VOLTAGE 3330000
		/* voltaje maximo de la PG2 es 4300000 con el cable USB*/
		#define USB_VOLTAGE 80000
		#eidef BATTERY_PG2V2
		/* voltaje maximo de la PG2V2 es 4270000 */
		#define MAX_VOLTAGE 4150000
		#define MIN_VOLTAGE 3330000
		/* voltaje maximo de la PG2v2 es 4321000 con el cable USB*/
		#define USB_VOLTAGE 110000
		#endif
		#endif
		#endif

      fscanf(batteryHandle, "%d", &battval);
      if (isBatteryCharging()) {
        battval=((battval - MIN_VOLTAGE) - USB_VOLTAGE) * 100 / (MAX_VOLTAGE - MIN_VOLTAGE);
      } else {
        battval=(battval - MIN_VOLTAGE) * 100 / (MAX_VOLTAGE - MIN_VOLTAGE);
      }
      fclose(batteryHandle);

      if(battval>100)
        battval=100;
      else if(battval<0)
        battval=0;

      // si el array está a 0, estamos leyendo por primera vez, actualizamos el array completo
      if(batt_average[battavg_idx]==0) {
        for(int f=0; f<10; f++)
          batt_average[f]=battval;
        return battval;
      }

      // actualizamos la casilla del array con el nuevo valor leído
      batt_average[battavg_idx++]=battval;
      if(battavg_idx>9)
        battavg_idx=0;
    }
  }

  // calculamos la media de los últimos 10 valores leídos
  battval=0;
  for(int f=0; f<10; f++)
    battval=battval+batt_average[f];
  battval=battval/10;

  return battval;
}

unsigned short isBatteryCharging()
{
	FILE *usbHandle = NULL;

#if defined(PLATFORM_A320) || defined(PLATFORM_GCW0) || defined(PLATFORM_NANONOTE)
	usbHandle = fopen("/sys/class/power_supply/usb/online", "r");
#endif
	if (usbHandle) {
		int usbval = 0;
		fscanf(usbHandle, "%d", &usbval);
		fclose(usbHandle);

		return usbval;
	}

	return 0;
}

Battery::Battery(SurfaceCollection& sc_)
	: sc(sc_)
{
	lastUpdate = SDL_GetTicks();
	update();
}

const OffscreenSurface *Battery::getIcon()
{
	// Check battery status every 60 seconds.
	unsigned int now = SDL_GetTicks();
	if (now - lastUpdate >= 60000) {
		lastUpdate = now;
		update();
	}

	return sc.skinRes(iconPath);
}

std::string Battery::getLevel()
{
  std::stringstream lv;
  lv << getBatteryLevel() << "%";  // no aparece el porcentaje (fix ninoh-fox)
  std::string value;
  lv >> value;
  return value;
}

void Battery::update()
{
	unsigned short battlevel = getBatteryLevel();
	if (isBatteryCharging()) {
		iconPath = "imgs/battery/ac.png";
	} else {
    int iconid=1+battlevel/20;
    if(iconid>5)
      iconid=5;
		std::stringstream ss;
		ss << "imgs/battery/" << iconid << ".png";   // rafa vico
		ss >> iconPath;
	}
}
