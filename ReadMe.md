Hexagon Grid Generator (HexGrid)

This project includes a simple generator that creates a 2 dimensional grid (or lattice) upon which vertices can be plotted and connected to form a hexagonal mesh.

The grid is composed of two std::vectors which represent two perpidicular axes, the major axis and minor axis, which can be interpreted as desired. (An example has y being major, and x being minor).

The grid is defined by the number of hexagons that can be formed along the major axis, and its "origin" is the corner of the plane it is being drawn on. (See the doxy for specifics)

This project also includes a mesh builder (HexMesh). The mesh builder will create a vertex array from a HexGrid which plots vertices at the appropriate points.

This project also includes a simple OpenGL renderer using SDL and GLEW to visualize the HexMesh.
