import sys
import time
import libvirt

num_vms = 2
num_running = 1
domNames = ['vm0', 'vm1']
runningDoms = ['vm0', 'vm1']
usage_history = [[0] for _ in range(5)]
threshold = 90 #% cpu time
boot_time = 30 #secs
timerRunning = False
timerTime = 0

def startTimer():
    global timerRunning, timerTime
    timerRunning = True
    timerTime = 0

def checkTimer():
    global timerRunning, timerTime
    if timerRunning:
        if timerTime >= boot_time:
            timerRunning = False
            return True 
        else:
            timerTime += 1
            print(timerTime)
    return False

def isOverloaded(usage):
    global usage_history
    usage_history = usage_history[1:] + [usage]
    return all([all([u>threshold for u in usg]) for usg in usage_history])

def getStats(dom):
    if dom.isActive():
        return dom.getCPUStats(True)
    else:
        return [{'cpu_time':0, 'user_time':0, 'system_time':0}]

def spawnNewVm(doms):
    global num_vms, num_running
    if timerRunning:
        return
    if num_running == num_vms:
        print("***** All vms running ******")
    else:
        print("******* starting " + domNames[num_running] + " *******")
        doms[num_running].create()
        startTimer()
        
def startNewVm():
    global num_running, runningDoms, domNames
    runningDoms.append(domNames[num_running])
    num_running += 1
    print("******* started " + domNames[num_running-1] + " ********")
    f = open("num_active", 'w')
    f.write(str(num_running))
    f.close()

conn = libvirt.open('qemu:///system')
if conn == None:
    print('Failed to open connection to qemu:///system', file=sys.stderr)
    exit(1)
doms = []
for domName in domNames:
    dom = conn.lookupByName(domName)
    if dom == None:
        print('Failed to find the domain '+domName, file=sys.stderr)
        exit(1)
    doms.append(dom)

stats = [getStats(dom) for dom in doms]
totaltime = time.time()
cputimes = [(stat[0]['cpu_time']-stat[0]['system_time']-stat[0]['user_time'])/1000000000.0 for stat in stats]
while(1):
    time.sleep(1)
    stats = [getStats(dom) for dom in doms]
    newcputimes = [stat[0]['cpu_time']/1000000000.0 for stat in stats]
    newtotaltime = time.time()
    usage = [100*(newcputimes[i]-cputimes[i])/(newtotaltime-totaltime) for i in range(num_running)]

    [print(runningDoms[i]+':', usage[i], '%') for i in range(num_running)]
    print('----------------------------')

    if isOverloaded(usage):
        spawnNewVm(doms)

    if checkTimer():
        startNewVm()
    
    totaltime = newtotaltime
    cputimes = newcputimes
     

conn.close()
exit(0)
