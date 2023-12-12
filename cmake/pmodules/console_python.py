import sys

def execute(script):
    namespace = sys.modules
    namespace["__builtins__"] = sys.modules["builtins"]
    
    from code import InteractiveConsole
    console = InteractiveConsole(locals=namespace,
                                 filename="<python_console>")
    
    import io
    stdout = io.StringIO()
    stderr = io.StringIO()
    
    # redirect output
    from contextlib import (
        redirect_stdout,
        redirect_stderr,
    )
    
    # not included with Python
    class redirect_stdin(redirect_stdout.__base__):
        _stream = "stdin"
    
    # don't allow the stdin to be used, can lock blender.
    with redirect_stdout(stdout), \
            redirect_stderr(stderr), \
            redirect_stdin(None):
    
        # in case exception happens
        line = ""  # in case of encoding error
        is_multiline = False
    
        try:
            line = script
    
            # run the console, "\n" executes a multi line statement
            line_exec = line if line.strip() else "\n"
    
            is_multiline = console.push(line_exec)
        except:
            # unlikely, but this can happen with unicode errors for example.
            import traceback
            stderr.write(traceback.format_exc())
    
    output = stdout.getvalue()
    output_err = stderr.getvalue()
    
    return output, output_err
