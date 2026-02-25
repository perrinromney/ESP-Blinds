# Tasmota Berry Script: Power-Managed 270° Servo (Final Version)
var pin_pwm = 4
var min_duty = 26
var max_duty = 128
var current_threshold = 45  # mA 

var move_watchdog = nil
var safety_timer = nil
var target_angle = 0

import json
import string

# Initialize
tasmota.cmd("PwmFrequency 50")

def finish_move()
    tasmota.log("SERVO: Powering OFF relay.", 3)
    tasmota.cmd("Power1 0")
    if move_watchdog != nil tasmota.remove_timer(move_watchdog) end
    if safety_timer != nil tasmota.remove_timer(safety_timer) end
    move_watchdog = nil
    safety_timer = nil
end

def monitor_current()
    var s = tasmota.read_sensors()
    # Use json.decode for C3 compatibility
    var sensors = json.decode(s)
    var current = 0
    
    if sensors != nil && sensors.contains('INA219')
        current = sensors['INA219']['Current']
    end

    if current != 0 && current < current_threshold
        finish_move()
    else
        # Re-queue the monitor
        move_watchdog = tasmota.set_timer(100, monitor_current)
    end
end

def execute_move()
    var duty = min_duty + (target_angle * (max_duty - min_duty) / 270)
    tasmota.cmd("PWM1 " + str(int(duty)))
    
    # Start checking current after 400ms
    move_watchdog = tasmota.set_timer(400, monitor_current)
    
    # Safety cut-off
    safety_timer = tasmota.set_timer(5000, finish_move)
end

def set_angle(cmd, idx, payload, payload_json)
    var angle = number(payload)
    if angle >= -135 && angle <= 135
        target_angle = angle + 135
        tasmota.log(f"SERVO: Moving to {target_angle} degrees (HA tilt {angle}).", 3)
        tasmota.cmd("Power1 1")
        tasmota.set_timer(50, execute_move)
        tasmota.resp_cmnd('{"ServoAngle":' + str(angle) + '}')
    else
        tasmota.resp_cmnd_error("Range -135 to 135")
    end
end

def publish_discovery()
    var wifi = tasmota.wifi()
    var mac = wifi['mac']
    
    var mac_with_colon = ""
    var mac_no_colon = ""
    
    if size(mac) == 12
        for i: 0..5
            mac_with_colon += mac[i*2..i*2+1]
            if i < 5 mac_with_colon += ":" end
        end
        mac_no_colon = mac
    else
        mac_with_colon = mac
        mac_no_colon = string.replace(mac, ":", "")
    end
    
    var topic = tasmota.cmd("Topic")['Topic']
    var prefix1 = tasmota.cmd("Prefix1")['Prefix1']
    var prefix2 = tasmota.cmd("Prefix2")['Prefix2']
    
    var discovery_topic = "homeassistant/cover/" + mac_no_colon + "_blinds/config"
    
    var payload = {
        "name": "Blinds Tilt",
        "command_topic": prefix1 + "/" + topic + "/servoAngle",
        "payload_open": "0",
        "payload_close": "-135",
        "tilt_command_topic": prefix1 + "/" + topic + "/servoAngle",
        "tilt_status_topic": prefix2 + "/" + topic + "/RESULT",
        "tilt_status_template": "{{ value_json.ServoAngle }}",
        "tilt_min": -135,
        "tilt_max": 135,
        "tilt_closed_value": -135,
        "tilt_opened_value": 0,
        "device": {
            "connections": [["mac", mac_with_colon]]
        },
        "unique_id": mac_no_colon + "_blinds_tilt"
    }
    
    tasmota.publish(discovery_topic, json.dump(payload), true)
end

tasmota.add_rule("mqtt#connected", publish_discovery)
tasmota.add_cmd('ServoAngle', set_angle)

tasmota.add_cmd('ServoAngle', set_angle)