// System automation example in Rubber Duck
// This demonstrates the specific task of the language

// Automatic variable declaration
Auto currentTime = get_current_time();
Auto userName = get_user_name();
Auto osInfo = get_os_info();

Display("Current time: " + currentTime);
Display("User name: " + userName);
Display("OS info: " + osInfo);

// File operations
Auto files = list_files("./");
for (Auto i = 0; i < files.size(); i = i + 1) {
    Display("File: " + files.get(i));
}

// System commands
Auto result = run_command("echo Hello from Rubber Duck");
Display("Command result: " + result);