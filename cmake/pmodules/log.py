import logging
import tempfile
import os
import sys
import pathlib
import shutil

def create_log(name):
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)
    handler1 = logging.StreamHandler()
    fmt1 = logging.Formatter(fmt="%(levelname)-9s %(message)s")
    handler1.setFormatter(fmt1)
    logger.addHandler(handler1)
    
    dir_name = get_datadir() / 'python_cache'
    
    try:
        dir_name.mkdir(parents=True)
    except OSError as error:
        logger.info('create_log OSError {}'.format(error))
        
    file_name = '{0}/{1}.log'.format(str(dir_name), name)
    handler2 = logging.FileHandler(filename=file_name,mode='w')
    
    fmt2 = logging.Formatter(fmt="%(asctime)s - %(name)s - %(levelname)-9s - %(filename)-8s : %(lineno)s line - %(message)s"
                        ,datefmt="%H:%M:%S")
    
    handler2.setFormatter(fmt2)
    logger.addHandler(handler2)
    
    logger.info('create_log in {}'.format(file_name))
    return logger

def get_clear_temp_dir(name)->pathlib.Path:
    dir_name = get_datadir() / name
    #shutil.rmtree(dir_name, ignore_errors=True)
    dir_name.mkdir(parents=True, exist_ok=True) 
    return dir_name
    
def get_datadir() -> pathlib.Path:

    """
    Returns a parent directory path
    where persistent application data can be stored.

    # linux: ~/.local/share
    # macOS: ~/Library/Application Support
    # windows: C:/Users/<USER>/AppData/Roaming
    """

    home = pathlib.Path.home()

    if sys.platform == "win32":
        return home / "AppData/Roaming"
    elif sys.platform == "linux":
        return home / ".local/share"
    elif sys.platform == "darwin":
        return home / "Library/Application Support"