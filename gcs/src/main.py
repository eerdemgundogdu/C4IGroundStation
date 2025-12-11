#!/usr/bin/env python3
"""
C4I Ground Control Station - Main Application
Tactical dashboard for multi-vehicle command and control
"""

import sys
import json
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QTableWidget, QTableWidgetItem, QTextEdit, QPushButton,
    QLabel, QGroupBox, QSplitter, QStatusBar, QComboBox
)
from PyQt6.QtCore import Qt, QTimer, pyqtSignal, QThread
from PyQt6.QtGui import QColor, QFont
import serial
import serial.tools.list_ports


class SerialWorker(QThread):
    data_received = pyqtSignal(dict)
    connection_changed = pyqtSignal(bool)
    
    def __init__(self, port, baudrate=115200):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = False
        self.serial = None
    
    def run(self):
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=0.1)
            self.connection_changed.emit(True)
            self.running = True
            
            while self.running:
                if self.serial.in_waiting:
                    line = self.serial.readline().decode('utf-8', errors='ignore').strip()
                    if line.startswith('STATUS:'):
                        try:
                            data = json.loads(line[7:])
                            self.data_received.emit(data)
                        except json.JSONDecodeError:
                            pass
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            self.connection_changed.emit(False)
        finally:
            if self.serial:
                self.serial.close()
    
    def stop(self):
        self.running = False
        self.wait()
    
    def send_command(self, cmd):
        if self.serial and self.serial.is_open:
            self.serial.write(f"CMD:{cmd}\n".encode())


class VehicleTable(QTableWidget):
    def __init__(self):
        super().__init__()
        self.setColumnCount(7)
        self.setHorizontalHeaderLabels([
            'ID', 'Type', 'State', 'Lat', 'Lon', 'Alt', 'Battery'
        ])
        self.setAlternatingRowColors(True)
        self.setStyleSheet("""
            QTableWidget {
                background-color: #1a1a2e;
                color: #00ff88;
                gridline-color: #333355;
                font-family: 'Consolas';
            }
            QHeaderView::section {
                background-color: #16213e;
                color: #00ff88;
                border: 1px solid #333355;
                padding: 4px;
            }
        """)
    
    def update_vehicles(self, vehicles):
        self.setRowCount(len(vehicles))
        type_names = ['UNK', 'QUAD', 'FIXED', 'GND', 'BOAT', 'SENS']
        state_names = ['OFFLINE', 'STANDBY', 'ACTIVE', 'RTL', 'EMERG', 'LOST']
        state_colors = {
            0: '#666666', 1: '#ffaa00', 2: '#00ff88',
            3: '#00aaff', 4: '#ff0000', 5: '#ff00ff'
        }
        
        for row, v in enumerate(vehicles):
            self.setItem(row, 0, QTableWidgetItem(str(v.get('id', '?'))))
            self.setItem(row, 1, QTableWidgetItem(type_names[v.get('type', 0)]))
            
            state_item = QTableWidgetItem(state_names[v.get('state', 0)])
            state_item.setForeground(QColor(state_colors.get(v.get('state', 0), '#ffffff')))
            self.setItem(row, 2, state_item)
            
            self.setItem(row, 3, QTableWidgetItem(f"{v.get('lat', 0):.6f}"))
            self.setItem(row, 4, QTableWidgetItem(f"{v.get('lon', 0):.6f}"))
            self.setItem(row, 5, QTableWidgetItem(f"{v.get('alt', 0):.1f}m"))
            
            bat_item = QTableWidgetItem(f"{v.get('bat', 0):.0f}%")
            bat = v.get('bat', 0)
            if bat < 20:
                bat_item.setForeground(QColor('#ff0000'))
            elif bat < 50:
                bat_item.setForeground(QColor('#ffaa00'))
            self.setItem(row, 6, bat_item)


class MissionLog(QTextEdit):
    def __init__(self):
        super().__init__()
        self.setReadOnly(True)
        self.setMaximumHeight(150)
        self.setStyleSheet("""
            QTextEdit {
                background-color: #0a0a1a;
                color: #00ff88;
                border: 1px solid #333355;
                font-family: 'Consolas';
                font-size: 11px;
            }
        """)
    
    def add_log(self, message, level='INFO'):
        from datetime import datetime
        colors = {'INFO': '#00ff88', 'WARN': '#ffaa00', 'ERROR': '#ff0000', 'CMD': '#00aaff'}
        color = colors.get(level, '#ffffff')
        timestamp = datetime.now().strftime('%H:%M:%S')
        self.append(f'<span style="color:{color}">[{timestamp}] [{level}] {message}</span>')
        self.verticalScrollBar().setValue(self.verticalScrollBar().maximum())


class CommandPanel(QGroupBox):
    command_sent = pyqtSignal(str)
    
    def __init__(self):
        super().__init__("Commands")
        self.setStyleSheet("""
            QGroupBox {
                color: #00ff88;
                border: 1px solid #333355;
                margin-top: 10px;
                padding-top: 10px;
            }
            QPushButton {
                background-color: #16213e;
                color: #00ff88;
                border: 1px solid #00ff88;
                padding: 8px 16px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #1a3a5c;
            }
            QPushButton:pressed {
                background-color: #00ff88;
                color: #1a1a2e;
            }
            QPushButton#emergency {
                border-color: #ff0000;
                color: #ff0000;
            }
        """)
        
        layout = QVBoxLayout()
        
        row1 = QHBoxLayout()
        self.target_combo = QComboBox()
        self.target_combo.addItem("All Vehicles", 0)
        for i in range(1, 9):
            self.target_combo.addItem(f"Vehicle {i}", i)
        row1.addWidget(QLabel("Target:"))
        row1.addWidget(self.target_combo)
        layout.addLayout(row1)
        
        row2 = QHBoxLayout()
        for btn_text, cmd in [("Start", 1), ("Stop", 2), ("RTL", 3)]:
            btn = QPushButton(btn_text)
            btn.clicked.connect(lambda checked, c=cmd: self.send_cmd(c))
            row2.addWidget(btn)
        layout.addLayout(row2)
        
        row3 = QHBoxLayout()
        for btn_text, cmd in [("Loiter", 4), ("Resume", 5), ("Land", 6)]:
            btn = QPushButton(btn_text)
            btn.clicked.connect(lambda checked, c=cmd: self.send_cmd(c))
            row3.addWidget(btn)
        layout.addLayout(row3)
        
        self.emergency_btn = QPushButton("EMERGENCY STOP")
        self.emergency_btn.setObjectName("emergency")
        self.emergency_btn.clicked.connect(lambda: self.send_cmd(99))
        layout.addWidget(self.emergency_btn)
        
        self.setLayout(layout)
    
    def send_cmd(self, cmd_type):
        target = self.target_combo.currentData()
        self.command_sent.emit(f"{target},{cmd_type},0,0,0")


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("C4I Ground Control Station")
        self.setGeometry(100, 100, 1400, 900)
        self.setStyleSheet("""
            QMainWindow {
                background-color: #0a0a1a;
            }
            QLabel {
                color: #00ff88;
            }
        """)
        
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QHBoxLayout(central)
        
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        
        map_placeholder = QLabel("MAP DISPLAY\n(OpenStreetMap Integration)")
        map_placeholder.setStyleSheet("""
            QLabel {
                background-color: #1a1a2e;
                border: 2px solid #333355;
                font-size: 18px;
                color: #666688;
            }
        """)
        map_placeholder.setAlignment(Qt.AlignmentFlag.AlignCenter)
        map_placeholder.setMinimumHeight(400)
        left_layout.addWidget(map_placeholder, 2)
        
        log_group = QGroupBox("Mission Log")
        log_group.setStyleSheet("QGroupBox { color: #00ff88; border: 1px solid #333355; }")
        log_layout = QVBoxLayout(log_group)
        self.mission_log = MissionLog()
        log_layout.addWidget(self.mission_log)
        left_layout.addWidget(log_group, 1)
        
        right_panel = QWidget()
        right_layout = QVBoxLayout(right_panel)
        
        status_group = QGroupBox("Vehicle Status")
        status_group.setStyleSheet("QGroupBox { color: #00ff88; border: 1px solid #333355; }")
        status_layout = QVBoxLayout(status_group)
        self.vehicle_table = VehicleTable()
        status_layout.addWidget(self.vehicle_table)
        right_layout.addWidget(status_group, 2)
        
        self.command_panel = CommandPanel()
        self.command_panel.command_sent.connect(self.send_command)
        right_layout.addWidget(self.command_panel, 1)
        
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(left_panel)
        splitter.addWidget(right_panel)
        splitter.setSizes([900, 500])
        main_layout.addWidget(splitter)
        
        self.statusBar = QStatusBar()
        self.setStatusBar(self.statusBar)
        self.statusBar.showMessage("Disconnected")
        self.statusBar.setStyleSheet("color: #ffaa00;")
        
        self.serial_worker = None
        self.connect_serial()
        
        self.mission_log.add_log("C4I Ground Control Station initialized", "INFO")
    
    def connect_serial(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if ports:
            self.serial_worker = SerialWorker(ports[0])
            self.serial_worker.data_received.connect(self.handle_telemetry)
            self.serial_worker.connection_changed.connect(self.update_connection)
            self.serial_worker.start()
            self.mission_log.add_log(f"Connecting to {ports[0]}...", "INFO")
        else:
            self.mission_log.add_log("No serial ports found - running in demo mode", "WARN")
            self.demo_timer = QTimer()
            self.demo_timer.timeout.connect(self.demo_update)
            self.demo_timer.start(1000)
    
    def demo_update(self):
        import random
        demo_data = {
            'vehicles': [
                {'id': 1, 'type': 1, 'state': 2, 'lat': 39.9 + random.random()*0.01,
                 'lon': 32.8 + random.random()*0.01, 'alt': 100 + random.random()*10,
                 'hdg': random.random()*360, 'spd': 10 + random.random()*5, 'bat': 85, 'sats': 12},
                {'id': 2, 'type': 1, 'state': 3, 'lat': 39.91, 'lon': 32.81, 'alt': 50,
                 'hdg': 180, 'spd': 5, 'bat': 42, 'sats': 10},
                {'id': 3, 'type': 3, 'state': 1, 'lat': 39.89, 'lon': 32.79, 'alt': 0,
                 'hdg': 90, 'spd': 0, 'bat': 67, 'sats': 8}
            ],
            'count': 3
        }
        self.handle_telemetry(demo_data)
    
    def handle_telemetry(self, data):
        self.vehicle_table.update_vehicles(data.get('vehicles', []))
        self.statusBar.showMessage(f"Connected - {data.get('count', 0)} vehicles online")
        self.statusBar.setStyleSheet("color: #00ff88;")
    
    def update_connection(self, connected):
        if connected:
            self.statusBar.showMessage("Connected")
            self.statusBar.setStyleSheet("color: #00ff88;")
            self.mission_log.add_log("Serial connection established", "INFO")
        else:
            self.statusBar.showMessage("Disconnected")
            self.statusBar.setStyleSheet("color: #ff0000;")
            self.mission_log.add_log("Serial connection lost", "ERROR")
    
    def send_command(self, cmd):
        self.mission_log.add_log(f"Sending command: {cmd}", "CMD")
        if self.serial_worker:
            self.serial_worker.send_command(cmd)
    
    def closeEvent(self, event):
        if self.serial_worker:
            self.serial_worker.stop()
        event.accept()


def main():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    font = QFont('Segoe UI', 10)
    app.setFont(font)
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())


if __name__ == '__main__':
    main()
