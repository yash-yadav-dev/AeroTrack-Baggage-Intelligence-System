\# AeroTrack – Airport Baggage Tracking System



A desktop-based Airport Baggage Tracking System developed using \*\*C++ and Qt 6\*\*.



AeroTrack provides a graphical interface for managing baggage records, monitoring baggage status, and exporting baggage manifests.



\## Features



\- Manage airport baggage records.

\- Track baggage status and status history.

\- Airline selection and baggage identification.

\- JSON-based data persistence.

\- Automatic backup of existing JSON data before saving.

\- Export baggage manifests to CSV.

\- Desktop GUI built with Qt Widgets.



\## Technologies Used



\- C++

\- Qt 6 (Core and Widgets)

\- CMake

\- JSON

\- CSV



\## Requirements



\- Qt 6.5 or later

\- CMake 3.19 or later

\- A compatible C++ compiler



\## Build Instructions



1\. Clone this repository.

2\. Open the project in Qt Creator.

3\. Configure the project using a compatible Qt Kit.

4\. Build and run the application.



Alternatively, use CMake:



```bash

cmake -S . -B build

cmake --build build

```



\## Data Storage



Baggage records are stored locally in `baggage\_data.json`. The application also supports CSV manifest export.



Runtime data and generated files are excluded from version control.



\## Author



Developed as an Advanced Data Structures project.



\---



\*\*AeroTrack – Smarter Airport Baggage Management.\*\*

