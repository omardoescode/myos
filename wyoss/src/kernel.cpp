// #define GRAPHICS
// #define NETWORKING_EX

#include "drivers/amd_am79c973.h"
#include "drivers/ata.h"
#include "net/icmp.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "syscall.h"
#include <common/types.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <filesystem/fat.h>
#include <filesystem/msdospart.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <memorymanagement.h>
#include <multitasking.h>
#include <net/arp.h>
#include <net/ethernetframe.h>
#include <net/ipv4.h>

#ifdef GRAPHICS
#include "gui/window.h"
#include <drivers/vga.h>
#include <gui/desktop.h>
#endif

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::net;
using namespace myos::filesystem;

#ifdef GRAPHICS
using namespace myos::gui;
#endif

static unsigned short *VideoMemory = (unsigned short *)0xb8000;

void printf(const char *str) {

  static uint8_t x = 0, y = 0;

  for (int i = 0; str[i] != '\0'; ++i) {
    switch (str[i]) {
    case '\n':
      y++, x = 0;
      break;
    case '\b':
      if (x > 0) {
        x--;
      } else if (y > 0) {
        y--;
        x = 79;
      }
      VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
      break;
    default:
      VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
      x++;
      break;
    }

    if (x >= 80) {
      y++;
      x = 0;
    }
  }

  if (y >= 25) {
    for (y = 0; y < 25; y++)
      for (x = 0; x < 80; x++)
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
    x = 0, y = 0;
  }
}

void printHex(uint8_t key) {
  char foo[] = "00";
  const char *hex = "0123456789ABCDEF";
  foo[0] = hex[(key >> 4) & 0x0F];
  foo[1] = hex[key & 0x0F];
  printf(foo);
}

typedef void (*constructor)();

extern "C" constructor *start_ctors;
extern "C" constructor *end_ctors;
extern "C" void callConstructors() {
  for (constructor *i = start_ctors; i != end_ctors; i++)
    (*i)();
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler {
  void OnKeyDown(char c) override {
    char foo[] = " ";
    foo[0] = c;
    printf(foo);
  }
};

class MouseToConsole : public MouseEventHandler {

  int x = 40, y = 12;

  void OnMouseDown(uint8_t button) override {
    VideoMemory[80 * y + x] = (((VideoMemory[80 * y + x] & 0xF000) >> 4) |
                               ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
                               (VideoMemory[80 * y + x] & 0x00FF));
  }
  void OnMouseMove(int xoffset, int yoffset) override {
    VideoMemory[80 * y + x] = (((VideoMemory[80 * y + x] & 0xF000) >> 4) |
                               ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
                               (VideoMemory[80 * y + x] & 0x00FF));

    x += xoffset;
    if (x < 0)
      x = 0;
    if (x >= 80)
      x = 79;

    y += yoffset;

    if (y < 0)
      y = 0;
    if (y >= 25)
      y = 24;

    VideoMemory[80 * y + x] = (((VideoMemory[80 * y + x] & 0xF000) >> 4) |
                               ((VideoMemory[80 * y + x] & 0x0F00) << 4) |
                               (VideoMemory[80 * y + x] & 0x00FF));
  }
};

void sysprintf(char *str) { asm("int $0x80" ::"a"(4), "b"(str)); }

void TaskA() {
  while (1) {
    sysprintf("Hello from Task A\n");
    // printf("From kernel??");
  }
}
void TaskB() {
  while (1) {
    sysprintf("Hello from Task B\n");
    // printf("From kernel??");
  }
}

static volatile char tcpRxBuf[4096];
static volatile uint32_t tcpRxHead = 0;
static volatile uint32_t tcpRxTail = 0;

class PrintfTCPHandler : public TransmissionControlProtocolHandler {
public:
  bool
  HandleTransmissionControlMessage(TransmissionControlProtocolSocket *socket,
                                   common::uint8_t *data,
                                   common::uint16_t size) override {
    for (int i = 0; i < size; i++) {
      uint32_t next = (tcpRxHead + 1) % sizeof(tcpRxBuf);
      if (next == tcpRxTail)
        break;
      tcpRxBuf[tcpRxHead] = (char)data[i];
      tcpRxHead = next;
    }

    if (size > 4 && data[0] == 'q' && data[1] == 'u' && data[2] == 'i' &&
        data[3] == 't') {
      socket->Disconnect();
    }

    return true;
  }
};

static void DrainTCPData() {
  if (tcpRxTail == tcpRxHead)
    return;
  printf("\n[TCP DATA]: ");
  char foo[2] = {0, 0};
  while (tcpRxTail != tcpRxHead) {
    foo[0] = tcpRxBuf[tcpRxTail];
    printf(foo);
    tcpRxTail = (tcpRxTail + 1) % sizeof(tcpRxBuf);
  }
}
class PrintfUDPHandler : public UserDatagramProtocolHandler {

public:
  void HandlerUserDataProtocolMessage(UserDatagramProtocolSocket *socket,
                                      common::uint8_t *data,
                                      common::uint16_t size) override {
    char *foo = " ";
    for (int i = 0; i < size; i++) {
      foo[0] = data[i];
      printf(foo);
    }
  }
};

class ATAInterruptHandler : public hardwarecommunication::InterruptHandler {
public:
  ATAInterruptHandler(hardwarecommunication::InterruptManager *mgr, uint8_t irq)
      : InterruptHandler(mgr, irq) {}
};

extern "C" void kernelMain(void *multiboot_structure,
                           uint32_t /*magicnumber*/) {
  for (int i = 0; i < 80 * 25; i++)
    VideoMemory[i] = (VideoMemory[i] & 0xFF00) | ' ';

  printf("Hello, World!\n");

  GlobalDescriptorTable gdt;

  uint32_t *memupper = (uint32_t *)(((size_t)multiboot_structure) + 8);
  size_t heap = 10 * 1024 * 1024;

  // ??
  MemoryManager memoryMgr(heap, (*memupper) * 1024 - heap - 10 * 1024);

  printf("heap: 0x");
  printHex((heap >> 24) & 0xFF);
  printHex((heap >> 16) & 0xFF);
  printHex((heap >> 8) & 0xFF);
  printHex((heap >> 0) & 0xFF);

  void *allocated = memoryMgr.malloc(1024);
  printf("\nallocated: 0x");
  printHex(((common::size_t)allocated >> 24) & 0xFF);
  printHex(((common::size_t)allocated >> 16) & 0xFF);
  printHex(((common::size_t)allocated >> 8) & 0xFF);
  printHex(((common::size_t)allocated >> 0) & 0xFF);
  printf("\n");

  TaskManager taskMgr;

  /*
  Task task1(&gdt, TaskA);
  Task task2(&gdt, TaskB);
  taskMgr.AddTask(&task1);
  taskMgr.AddTask(&task2);
  */

  InterruptManager interruptMgr(0x20, &gdt, &taskMgr);
  SyscallHandler syscalls(&interruptMgr, 0x80);

  printf("Initializing Drivers\n");
  DriverManager drvMgr;

#ifdef GRAPHICS
  Desktop desktop(320, 200, 0x00, 0x00, 0xA8);

  KeyboardDriver keyboard(&interruptMgr, &desktop);
  MouseDriver mouse(&interruptMgr, &desktop);

  VideoGraphicsArray vga;
#else
  PrintfKeyboardEventHandler keyboardHandler;
  KeyboardDriver keyboard(&interruptMgr, &keyboardHandler);

  MouseToConsole mouseHandler;
  MouseDriver mouse(&interruptMgr, &mouseHandler);
#endif

  drvMgr.AddDriver(&keyboard);
  drvMgr.AddDriver(&mouse);

  PeripheralComponentInterconnectController PCIController;
  PCIController.SelectDrivers(&drvMgr, &interruptMgr);

  printf("Activating Drivers\n");
  drvMgr.ActivateAll();
  printf("Drivers Activated\n");

  // interrupt 14
  AdvancedTechnologyAttachment ata0m(true, 0x1F0);
  printf("ATA Primary Master: ");
  ata0m.Identify();

  AdvancedTechnologyAttachment ata0s(false, 0x1F0);
  printf("ATA Primary Slave: ");
  ata0s.Identify();

  filesystem::MSDOSPartitionTable::ReadPartition(&ata0m);

  {
    filesystem::MasterBootRecord mbr;
    ata0m.Read28(0, (uint8_t *)&mbr, sizeof(mbr));
    for (int p = 0; p < 4; p++) {
      uint8_t id = mbr.primaryPartition[p].partition_id;
      if (id != 0x0B && id != 0x0C)
        continue;
      filesystem::Fat32 fs(&ata0m, mbr.primaryPartition[p].start_lba);

      const char *msg = "written by wyoss kernel\n";
      uint32_t len = 0;
      while (msg[len])
        len++;
      printf("\nWriting NEW.TXT...\n");
      fs.WriteFile("NEW.TXT", (const uint8_t *)msg, len);

      filesystem::DirEntry entries[16];
      int32_t n = fs.ReadDirectory(entries, 16);
      printf("Root after write:\n");
      for (int32_t k = 0; k < n; k++) {
        printf("  ");
        printf(entries[k].name);
        printf("\n");
      }

      uint8_t rb[64];
      int32_t got = fs.ReadFile("NEW.TXT", rb, sizeof(rb) - 1);
      if (got >= 0) {
        rb[got] = 0;
        printf("NEW.TXT: ");
        printf((const char *)rb);
      }
      break;
    }
  }

  /*
  ATAInterruptHandler ataPrimary(&interruptMgr,
                                 interruptMgr.HardwareInterruptOffset() +
  0x0E); ATAInterruptHandler ataSecondary( &interruptMgr,
  interruptMgr.HardwareInterruptOffset() + 0x0F);

  ata0s.Write28(0, (uint8_t *)"http://www.AlgorithMan.de", 25);
  ata0s.Flush();
  ata0s.Read28(0);

  // interrupt 15
  AdvancedTechnologyAttachment ata1m(true, 0x170);
  AdvancedTechnologyAttachment ata1s(false, 0x170);

  // third at 0x1E8, and fourth is at 0x168
  */

#ifdef NETWORKING_EX
  amd_am79c973 *eth0 = (amd_am79c973 *)(drvMgr.drivers[2]);

  uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15; // will qemu accept this iP?
  uint32_t ip_be = ((uint32_t)ip4 << 24) | ((uint32_t)ip3 << 16) |
                   ((uint32_t)ip2 << 8) | ((uint32_t)ip1);

  uint8_t gip1 = 10, gip2 = 0, gip3 = 2,
          gip4 = 2; // IP gateway. what is a gateway ??
  uint32_t gip_be = ((uint32_t)gip4 << 24) | ((uint32_t)gip3 << 16) |
                    ((uint32_t)gip2 << 8) | ((uint32_t)gip1);

  uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255,
          subnet4 = 0; // IP gateway. what is a gateway ??
  uint32_t subnet_be = ((uint32_t)subnet4 << 24) | ((uint32_t)subnet3 << 16) |
                       ((uint32_t)subnet2 << 8) | ((uint32_t)subnet1);
  eth0->SetIPAddress(ip_be);

  EtherFrameProvider etherframe(eth0);

  AddressResolutionProtocol arp(&etherframe);

  InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnet_be);

  InternetControlMessageProtocol icmp(&ipv4);

  UserDatagramProtocolProvider udp(&ipv4);

  TransmissionControlProtocolProvider tcp(&ipv4);

  interruptMgr.Activate();

  arp.BroadcastMacAddress(gip_be);
  // icmp.RequestEchoReply(gip_be);

  // UserDatagramProtocolSocket *udpsocket = udp.Connect(gip_be, 1234);
  // PrintfUDPHandler udphandler;
  // udp.Bind(udpsocket, &udphandler);
  //
  //   udpsocket->Send((uint8_t *)"Hello UDP\n", 10);

  printf("\n\n\n\n");
  PrintfTCPHandler tcphandler;
  TransmissionControlProtocolSocket *tcpsocket = tcp.Connect(gip_be, 1234);
  tcp.Bind(tcpsocket, &tcphandler);
  tcpsocket->Send((uint8_t *)"Hello TCP\n", 10);

#endif

#ifdef GRAPHICS
  vga.SetMode(320, 200, 8);

  Window win1(&desktop, 10, 10, 20, 20, 0xA8, 0x00, 0x00);
  Window win2(&desktop, 40, 15, 30, 30, 0x00, 0xA8, 0x00);

  desktop.AddChild(&win1);
  desktop.AddChild(&win2);

  while (1) {
    desktop.Draw(&vga);
  }
#endif
  while (1) {
    myos::drivers::DrainRxLog();
    DrainTCPData();
    asm("hlt");
  }
}
