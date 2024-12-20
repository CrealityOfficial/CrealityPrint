import sys
from pathlib import Path

class ProfileData():
    def __init__(self, name, xdatas, ydatas):
        self.name = name
        self.xdatas = xdatas
        self.ydatas = ydatas
        
class Profile():
    '''
    '''
    def __init__(self, logger):
        self.logger = logger
        
    def show(self, datas):
        import matplotlib.pyplot as plt
        import numpy as np
        
        plt.figure()
        for data in datas:
            plt.plot(data.xdatas, data.ydatas, label=data.name)
            
        #ax.set(xlabel='time (s)', ylabel='memory (M)',
        #    title='About as simple as it gets, folks')
        #ax.grid()
        plt.legend()
        plt.show()
        
    def show_from_json(self, json_file):
        profile_datas = self._get_json_data(json_file)
        self.show(profile_datas)
        
    def _get_json_data(self, json_file):
        self.logger.info("load json : {}".format(json_file))
        
        import json
        profile_datas = []
        with open(json_file, 'r') as f:
            data = json.load(f)
            ticks = data["datas"]
            
            profile_data = ProfileData("", [], [])
            for tick in ticks:
                tag = tick["tag"]
                t = tick["time"]
                m = tick["memory"]
                
                if tag == "start":
                    profile_data = ProfileData("", [], [])
                elif "name" in tag:
                    profile_data.name = tag
                elif "end" == tag:
                    profile_datas.append(profile_data)
                 
                profile_data.xdatas.append(float(t))
                profile_data.ydatas.append(float(m) / 1024.0 / 1024.0)
            
            for pd in profile_datas:
                self.logger.info("profile data : {} have {}".format(pd.name, len(pd.xdatas)))

        return profile_datas
        
if __name__ == "__main__":
    json_file = sys.argv[1]
    
    import log
    logger = log.create_log('profile')

    profile = Profile(logger)
    profile.show_from_json(json_file)
    