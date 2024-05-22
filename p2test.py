import subprocess
import time

def run_btide_server(config_file):
    """This function will run the btide as a server that awaits commands."""
    process = subprocess.Popen(['./btide', config_file],
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               text=True)
    return process

def start_btide_client(config_file):
    """Starts a btide client process."""
    client_process = subprocess.Popen(['./btide', config_file],
                                      stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      text=True)
    return client_process

def send_commands_to_client(client_process, commands):
    """Sends a list of commands to an already running client process."""
    for command in commands:
        client_process.stdin.write(command + "\n")
        client_process.stdin.flush()

def extract_output(client_process, server_process, output_path):
    # Read output from client
    client_output = client_process.stdout.read()

    # Save outputs to files
    with open(output_path, 'w') as f:
        f.write(client_output)

    # Wait for both processes to terminate
    server_process.wait()
    client_process.wait()

def compare_files(output_file, expected_file):
    """Compare two files to check if they are identical."""
    try:
        with open(output_file, 'r') as file1, open(expected_file, 'r') as file2:
            contents1 = file1.readlines()
            contents2 = file2.readlines()

        if contents1 == contents2:
            return True
        else:
            return False
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return False

# Main execution
if __name__ == "__main__":
    # Test 1: BASIC PACKAGE COMMANDS
    commands = [
        "ADDPACKAGE pkg1.bpkg",
        "PACKAGES",
        "REMPACKAGE 5105d1a7ffb",
        "PACKAGES",
        "REMPACKAGE 3cf007c14ded16ab85d168fcf9d9b20effef7a0b8f89524d72eeaea97832a3193f9aa9528ebd0",
        "PACKAGES"
    ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(client_process, commands)
    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()

    extract_output(client_process, server_process, "tests/test01/test1.out")

    # Compare the output against the expected file
    comparison_result = compare_files("part2_tests/test01/test1.out", "part2_tests/test01/test1.expected")

    # Print comparison result or take further action based on the result
    print("Test 1:", "Passed" if comparison_result else "Failed")

    #Test 2: BASIC PEERS COMMANDS
    commands = [
        "PEERS",
        "CONNECT 127.0.0.1:9856",
        "PEERS",
        "DISCONNECT 127.0.0.1:9856",
        "PEERS"
    ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(client_process, commands)
    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()

    extract_output(client_process, server_process, "part2_tests/test02/test2.out")

    comparison_result = compare_files("part2_tests/test02/test2.out", "part2_tests/test02/test2.expected")

    print("Test 2:", "Passed" if comparison_result else "Failed")

    #Test 3: Already connected to peer
    commands = [
        "CONNECT 127.0.0.1:9856",
        "CONNECT 127.0.0.1:9856",
        "PEERS",
    ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(client_process, commands)
    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()

    extract_output(client_process, server_process, "part2_tests/test03/test3.out")

    comparison_result = compare_files("part2_tests/test03/test3.out", "part2_tests/test03/test3.expected")

    print("Test 3:", "Passed" if comparison_result else "Failed")

    #Test 4: Fetching data that peer does not have
    commands = [
        "CONNECT 127.0.0.1:9856",
        "ADDPACKAGE pkg3.bpkg",
        "FETCH 127.0.0.1:9856 5105d1a7ffbde836fe5aaa6704ecc751857561c6073 2c87207bc909188bb45904db002f7eb6da05e5d3031fbf0834a230d1d3691c61"
    ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(client_process, commands)

    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()
        
    extract_output(client_process, server_process, "part2_tests/test04/test4.out")

    comparison_result = compare_files("part2_tests/test04/test4.out", "part2_tests/test04/test4.expected")

    print("Test 4:", "Passed" if comparison_result else "Failed")

    #Test 5: Requesting chunk that does not exist
    commands = [
        "CONNECT 127.0.0.1:9856",
        "ADDPACKAGE pkg3.bpkg",
        "FETCH 127.0.0.1:9856 5105d1a7ffbde836fe5aaa6704ecc751857561c6073 2c8721abc90918ajh45904db002f7eb6da05f5d3031fbf0834a230d1d3691c61"
    ]

    # commands_server = [ #Commands for the client that is being connected to
    #     "ADDPACKAGE part2_tests/test05/test5.bpkg"
    # ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    #send_commands_to_client(server_process, commands_server)
    send_commands_to_client(client_process, commands)

    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()
        
    extract_output(client_process, server_process, "part2_tests/test05/test5.out")

    comparison_result = compare_files("part2_tests/test05/test5.out", "part2_tests/test05/test5.expected")

    print("Test 5:", "Passed" if comparison_result else "Failed")

    #Test 6: Invalid Commands
    commands = [
        "",
        "RANDOM COMMANDS",
    ]

    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(client_process, commands)
    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()

    extract_output(client_process, server_process, "part2_tests/test06/test6.out")

    comparison_result = compare_files("part2_tests/test06/test6.out", "part2_tests/test06/test6.expected")

    print("Test 6:", "Passed" if comparison_result else "Failed")

    #Test 7: Valid Fetch
    commands = [
        "CONNECT 127.0.0.1:9856",
        "ADDPACKAGE pkg1.bpkg",
        "FETCH 127.0.0.1:9856 3cf007c14ded16ab85d168fcf9d9b20effef7a0b8f89524d72eeaea97832a3193f9aa9528ebd0 c22250db8c3cdb65bc5e722dc408651cbb09a87ec3fb1f0555bb26a1fab96892"
    ]  
    
    commands_server = [ #Commands for the client that is being connected to
        "ADDPACKAGE btide_test2/pkg1.bpkg"
    ]
    
    server_process = run_btide_server('config2.cfg')
    client_process = start_btide_client('config1.cfg')

    send_commands_to_client(server_process, commands_server)
    send_commands_to_client(client_process, commands)

    client_process.stdin.write("QUIT\n")
    client_process.stdin.flush()
    server_process.stdin.write("QUIT\n")
    server_process.stdin.flush()

    extract_output(client_process, server_process, "part2_tests/test07/test7.out")

    comparison_result = compare_files("part2_tests/test07/test7.out", "part2_tests/test07/test7.expected")

    print("Test 7:", "Passed" if comparison_result else "Failed")