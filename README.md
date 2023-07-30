# Postopek
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](./LICENSE)
[![Build CI](https://github.com/pralad-p/Postopek/actions/workflows/cmake.yml/badge.svg?branch=master)](https://github.com/pralad-p/Postopek/actions/workflows/cmake.yml)

Postopek is a Procedure CLI application for Windows, designed for handling checklists with a unique
time-tracking feature. It was made to be minimalist and allows to focus on one task to its completion.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Features](#features)
- [Contributing](#contributing)

## Installation

### Prerequisites

- CMake (version 3.16 or higher)
- A C++ compiler that supports C++17
- Git
- PowerShell (7 preferably) with admin privileges

### Building from Source

1. Clone the repository to your local machine:

```bash
git clone https://github.com/pralad-p/Postopek.git
```

2. Navigate to the project directory:

```bash
cd Postopek
```

3. Create a new directory for the build:

```bash
mkdir build
```

4. Navigate to the build directory:

```bash
cd build
```

5. Run CMake to generate the Makefile:

```bash
cmake ..
```

6. Build the project:

```bash
make
```

## Usage

*To be done*

(Provide instructions on how to use the application, perhaps with example commands or screenshots)

## Project Structure

*To be done*

The project is structured as follows:

- `src`: Contains source and header files
- `main.cpp`: Serves as the entry point

## Features

*To be done*

- Big clock TUI: (Elaborate on this feature)
- Interactivity: (Elaborate on this feature)
- Checkboxes: Responsive to mouse/keyboard. Ticking a checkbox modifies the checkbox text, prepending the HH:MM time.
  Checkbox and text color change as it is ticked and unticked.
- Hovering: Hovering over a checkbox reveals comments.
- Input window: Adding text in the input window changes the hovering text. Confirming added text prepends a time-stamp
  as well.
- Text points: Added texts are in the form of points.

## Contributing

New contributors and/or feature requesters can view [the guide](./CONTRIBUTING.md). 