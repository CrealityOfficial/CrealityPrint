import os
import sys, getopt
import tempfile
import shutil
from xml.etree import ElementTree as ET

class Vertex(object):
    """
    �ڵ����
    """
    def __init__(self, key):
        self.key = key
        self.connectedTo = {}    # ���ָ��������ڵ㣬��Vertex�����ӱ�weight��

    def addNeighbor(self, nbr, weight):
        self.connectedTo.update({nbr: weight})

    def __str__(self):
        return str(self.key) + '-->' + str([nbr.key for nbr in self.connectedTo])

    def getConnections(self):
        return self.connectedTo.keys()

    def getId(self):
        return self.key

    def getWeight(self, nbr):
        weight = self.connectedTo.get(nbr)
        if weight is not None:
            return weight
        else:
            raise KeyError("No such nbr exist!")


class Graph(object):
    """
    ͼ����
    """
    def __init__(self):
        self.vertexList = {}         # �ֵ䱣��ڵ���Ϣ����{key: Vertex}�ķ�ʽ
        self.vertexNum = 0           # ͳ��ͼ�ڵ���

    def addVertex(self, key):
        self.vertexList.update({key: Vertex(key)})
        self.vertexNum += 1

    def getVertex(self, key):
        vertex = self.vertexList.get(key)
        return vertex

    def __contains__(self, key):
        return key in self.vertexList.values()

    def addEdge(self, f, t, weight=0):
        f, t = self.getVertex(f), self.getVertex(t)
        if not f:
            self.addVertex(f)
        if not t:
            self.addVertex(t)
        f.addNeighbor(t, weight)

    def getVertices(self):
        return self.vertexList.keys()

    def __iter__(self):
        return iter(self.vertexList.values())

def toGraphName(name, version):
    n = name + "-" + version
    return n 
    
def load_graph_from_xml(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    Graph G
    libs = root.findall("lib")
    for lib in libs:
        sublibs = lib.findall("sublib")
        name = lib.attrib["name"]
        version = lib.attrib["version"]
        
        if len(sublibs) == 0:
            G.addVertex(toGraphName(name, version))
            
        for sub in sublibs:
            n = sub.attrib["name"]
            v = sub.attrib["version"]
            G.addEdge(toGraphName(name, version), toGraphName(n, v))
    
    return G

