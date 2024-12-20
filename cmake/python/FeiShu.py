# -*- coding: utf-8 -*-
import requests
import sys
import os
import copy

class SendNotice():
    def __init__(self, data):
        self.url = data['url']
        self.name = data['name']
        self.download_url = data['download_url']
        self.divs = data['divs']
        
        self.method = 'post'
        self.headers = {
            'Content-Type': 'application/json'
        }
        self.json = {
            "msg_type": "interactive",
            "card": {
                "config": {
                    "wide_screen_mode": True,
                    "enable_forward": True
                },
                "elements": [{
                    "actions": [{
                        "tag": "button",
                        "text": {
                            "content": "查看测试报告",
                            "tag": "lark_md"
                        },
                        "url": self.download_url,
                        "type": "default",
                        "value": {}
                    }
                    ],
                    "tag": "action"
                }],
                "header": {
                    "title": {
                        "content": self.name,
                        "tag": "plain_text"
                    }
                }
            }
        }
        
        self.json['card']['elements'] = self.divs + self.json['card']['elements']

    def send_feishu(self):
        print('start send to feishu {}'.format(self.url))
        print('download_url {}'.format(self.download_url))
        requests.request(method=self.method, url=self.url, headers=self.headers, json=self.json)
        
### notice is a dict
### url
### name
### download_url
### datas is a list of dict {'input', 'value', 'state'}
def send_auto_test_notice(notice):
    protype = {
            "tag": "column_set",
            "flex_mode": "bisect",
            "background_style": "grey",
            "horizontal_spacing": "default",
            "columns": [
                {
                    "tag": "column",
                    "width": "weighted",
                    "weight": 1,
                    "elements": [
                        {
                        "tag": "markdown",
                        "text_align": "center",
                        "content": ""
                        }
                    ]
                },
                {
                    "tag": "column",
                    "width": "weighted",
                    "weight": 1,
                    "elements": [
                        {
                        "tag": "markdown",
                        "text_align": "center",
                        "content": ""
                        }
                    ]
                },
                {
                    "tag": "column",
                    "width": "weighted",
                    "weight": 1,
                    "elements": [
                        {
                        "tag": "markdown",
                        "text_align": "center",
                        "content": ""
                        }
                    ]
                }
            ]
        }
        
    header_div = copy.deepcopy(protype)
    header_div['columns'][0]['elements'][0]['content'] = "**输入**"
    header_div['columns'][1]['elements'][0]['content'] = "**结果**"
    header_div['columns'][2]['elements'][0]['content'] = "**细节**"
    
    divs = []
    divs.append(header_div)

    for data in notice['datas']:
        item = copy.deepcopy(protype)
        state = "<font color='green'>PASS</font>"
        if data['state'] == False:
            state = "<font color='red'>FAILED</font>"
    
        item['columns'][0]['elements'][0]['content'] = data['input']
        item['columns'][1]['elements'][0]['content'] = state
        #item['columns'][2]['elements'][0]['content'] = data['value']
        divs.append(item)

    notice['divs'] = divs
    SendNotice(notice).send_feishu()