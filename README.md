# Smart Carpooling System

A full-stack college project with a C++17 matching engine, Crow REST API, and React frontend.

## What is implemented

- Road edges store sourced distances in kilometres; no edges are loaded until routing data is verified.
- Dijkstra's algorithm finds shortest-distance paths and reconstructs the landmark sequence.
- Estimated travel time uses 30 km/h where road-specific speed data is unavailable.
- Two matching entry points:
  - `findDriversForPassenger`: a passenger searches available drivers.
  - `findPassengersForDriver`: a driver searches compatible passengers.
- The same matching engine evaluates both cases.
- The engine tries inserting the passenger pickup and drop-off into the driver's route.
- Hard constraints: seat capacity, passenger pickup window, driver's maximum wait, and driver's maximum detour.
- Matches are ranked by smallest detour, then smallest passenger wait.
- Booking confirmation, cancellation, and seat availability updates.
- React forms and match/booking screens.
- No CSV or database is required for this prototype.

## Project structure

```text
SmartCarpool/
├── CMakeLists.txt
├── CMakePresets.json
├── backend/
│   ├── include/
│   │   ├── Models.h
│   │   ├── Graph.h
│   │   ├── RoadNetworkData.h
│   │   ├── Matching.h
│   │   └── CarpoolService.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── Graph.cpp
│   │   ├── Matching.cpp
│   │   └── CarpoolService.cpp
│   └── tests/
│       └── GraphTests.cpp
└── frontend/
    ├── package.json
    ├── index.html
    ├── vite.config.js
    └── src/
        ├── main.jsx
        ├── App.jsx
        └── style.css
```

## Requirements

- Windows 10/11
- CMake 3.20+
- A C++17 compiler (Visual Studio Build Tools or MinGW-w64)
- Git (CMake downloads Crow from its official repository during configuration)
- Node.js 18+

## Run the C++ backend

Open PowerShell in the `SmartCarpool` folder:

```powershell
cmake --preset ucrt64
cmake --build --preset ucrt64-release
.\build-ucrt64\smartcarpool.exe
```

The API starts at `http://localhost:18080`.

Check it in a browser:

- `http://localhost:18080/api/health`
- `http://localhost:18080/api/locations`
- `http://localhost:18080/api/roads`
- `http://localhost:18080/api/drivers`
- `http://localhost:18080/api/passengers`

The first CMake configuration needs internet access to fetch Crow.

## Run the React frontend

Open a second PowerShell window:

```powershell
cd frontend
npm install
npm run dev
```

Open the local URL printed by Vite, normally `http://localhost:5173`.

The Vite development server forwards `/api` requests to the C++ backend at port 18080.

## Try the sample data

The backend starts with two sample drivers and two sample passengers. In the UI:

1. Select **I need a ride**, choose a passenger, and click **Find compatible journeys**.
2. Or select **I have a car**, choose a driver, and search for passengers.
3. Confirm a match to create a booking.
4. Cancel the booking to release the reserved seat.

You can also add new drivers and passengers through the forms.

## Important time-window distinction

- Passenger pickup window = preferred pickup time minus early tolerance through preferred pickup time plus late tolerance.
- Driver maximum wait = how long the driver agrees to wait if the driver reaches the pickup point before the passenger's earliest acceptable time.

These are separate constraints.

## Road data and travel-time assumptions

The main graph contains only these eight carpool pickup/drop-off landmarks, in this order:

1. JIIT Sector 128, Noida (primary starting point)
2. Botanical Garden Metro Station, Noida
3. Noida Sector 61, Noida
4. Noida Sector 59, Noida
5. Noida Sector 62, Noida
6. Noida Electronic City Metro Station, Noida
7. JIIT Sector 62, Noida
8. Indirapuram Habitat Centre, Ghaziabad

JIIT Sector 62 and Noida Sector 62 are separate locations. These are carpool road-network landmarks, not a metro route sequence.

Sourced route data is stored separately from Dijkstra in `backend/include/RoadNetworkData.h`. The eight locations were geocoded with Photon (OpenStreetMap data), then routed with the OSRM public Route API on 2026-11-23. The data contains only route results returned by OSRM: 56 directed landmark-to-landmark routes and the alternative routes returned by the service. Each direction is stored independently because road-route distances can differ by direction; alternative routes are separate graph edges. Route endpoints are snapped by OSRM to nearby drivable roads.

Dijkstra minimizes the total stored road distance in kilometres and reconstructs the graph-location sequence. Estimated time is separate: where road-specific speed data is unavailable, the average car speed is 30 km/h and estimated minutes are rounded up from `distanceKm / 30 * 60`. This is not actual traffic-aware travel time. The stored OSRM routes are not turn-by-turn navigation or live-traffic estimates.

## Current prototype limitations

- Data is stored in memory. Restarting the backend resets sample and newly entered users/bookings.
- No authentication, payment, live GPS tracking, notifications, or persistent database yet.
- The route shown is a sequence of graph locations, not turn-by-turn navigation.
- Time calculations assume the same service day; overnight journeys need explicit date handling.
- This is a functional prototype foundation, not a production ride-sharing service.
