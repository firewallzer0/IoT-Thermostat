#ifndef NTP_H_INCLUDED
#define NTP_H_INCLUDED
/*
 * NTP.h
 *
 * Author:   Hiromasa Ihara (taisyo)
 * Created:  2016-04-22
 */

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>

#define NTP_DEFAULT_TIMEOUT_MS 5000
#define NTP_PORT 123

class NTPClient
{
  public:
    void begin(){
    }
    int getTime(String server_host, uint32_t *timestamp1970){
      DNSClient dns;
      IPAddress server_ip;
      uint32_t timestamp1900; // ntp timestamp

      dns.begin(Ethernet.dnsServerIP());
      dns.getHostByName(server_host.c_str(), server_ip);

      Udp.begin(1024+(millis() & 0xFF));
      sendPacket(server_ip);
      recvPacket(NTP_DEFAULT_TIMEOUT_MS, timestamp1900);
      *timestamp1970 = convert1900to1970(timestamp1900);

      return 0;
    }

  private:

    uint32_t convert1900to1970(uint32_t timestamp1900)
    {
      return timestamp1900 - 2208988800UL;
    }

    int sendPacket(IPAddress server_ip){
      Udp.beginPacket(server_ip, NTP_PORT);
      uint8_t oneByteBuf;

      oneByteBuf = 0b00001011;
      Udp.write(&oneByteBuf, 1);
      for (int i = 1; i < 48; i++) {
        oneByteBuf = 0;
        Udp.write(&oneByteBuf, 1);
      }
      Udp.endPacket();
    }

    int recvPacket(unsigned long timeout_ms, uint32_t &timestamp1900){
      unsigned int start_ms = millis();
      uint8_t oneByteBuf;

      while(!Udp.parsePacket()){
        if(millis() - start_ms > timeout_ms){
          return -1;
        }
      }

      for (int i = 0; i < 40; i++) {
        Udp.read(&oneByteBuf, 1);
      }
      timestamp1900 = 0;
      for (int i = 40; i < 44; i++) {
        Udp.read(&oneByteBuf, 1);
        timestamp1900 <<= 8;
        timestamp1900 += oneByteBuf;
      }

      return 0;
    }

    EthernetUDP Udp;
};

#endif   /* NTP_H_INCLUDED */
