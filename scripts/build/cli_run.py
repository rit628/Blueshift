import env
from util import *
import os
from pathlib import Path
from scapy.all import sniff, conf, sendp
from threading import Thread
from time import sleep
from subprocess import check_output

def broadcast_sniffer():
    ifaces = list(conf.ifaces.keys())
    send_ifaces = set(ifaces)
    broadcast_filter = f"udp and dst port {env.BROADCAST_PORT}"
    # listen on all interfaces for broadcast traffic and choose first sucessful interface as source
    source_iface = sniff(filter=broadcast_filter, iface=ifaces, count=1)[0].sniffed_on
    send_ifaces.remove(source_iface) # forward to all other interfaces

    def forward_broadcast(packet):
        for iface in send_ifaces:
            sendp(packet, iface, verbose=False)
    
    sniff(filter=broadcast_filter, prn=forward_broadcast,
          store=False, iface=source_iface)

def run(args):
    if args.local:
        driver, target = args.driver.split(":") if args.driver else (None, None)
        executable = Path(".", get_target_dir(target), env.RUNTIME_OUTPUT_DIRECTORY, args.binary)
        run_cmd([driver, executable, *args.binary_args])
    else:
        if args.vnc: initialize_vnc_display()
        if args.packet_forward:
            context = check_output(["docker", "context", "show"], text=True).strip()
            if context == "rootless":
                raise PermissionError("Packet forwarding only possible in rootful context. Try running again with elevated privileges or perform a manual context switch before running.")
            os.environ["NETWORK_MODE"] = "host"
            if args.udp_broadcast_forward:
                Thread(target=broadcast_sniffer, daemon=True).start()
                sleep(.25) # wait before running initialize_host() to ensure sniffing privilege is retained
        initialize_host()
        run_cmd(["docker", "compose", "run", "--rm", "builder", "run", "-l", args.binary, *args.binary_args])