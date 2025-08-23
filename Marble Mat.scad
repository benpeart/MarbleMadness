// =========================
// Hole Plate with Parametric Standoffs (Inset Perimeter)
// =========================

// --- Plate parameters ---
num_x       = 4;
num_y       = 4;
pla_shrink  = 0.2;       // mm that printing shrinks the hole
marble_d    = 17;        // mm diameter of largest marble (sphere)
hole_d      = 15.5 + pla_shrink; // mm slightly smaller than the diameter of the smallest marble
pitch       = 16.67;     // mm (center-to-center)
margin      = 10;        // mm edge margin beyond outer hole edges
thickness   = 3;         // mm
led_height  = 2.15;      // mm
standoff    = led_height + marble_d / 2; // height of standoffs
standoff_d  = 3;         // diameter of standoffs

// --- Quality settings ---
$fn_fast    = 64;        // low-poly for speed
$fn_final   = 128;       // smooth for export

// --- Auto quality based on preview mode ---
$fn_holes   = $preview ? $fn_fast : $fn_final;

module hole_plate(num_x, num_y, marble_d, hole_d, pitch, margin, thickness, standoff, standoff_d) {
    grid_span_x = (num_x - 1) * pitch;
    grid_span_y = (num_y - 1) * pitch;

    plate_x = grid_span_x + hole_d + 2 * margin;
    plate_y = grid_span_y + hole_d + 2 * margin;

    // Calculate vertical offset so sphere cuts hole_d at top face
    R = marble_d / 2;
    r = hole_d / 2;
    offset = sqrt(R*R - r*r);

    union() {
        // Plate with holes
        difference() {
            cube([plate_x, plate_y, thickness], center = true);

            // Holes
            for (i = [0 : num_x - 1])
                for (j = [0 : num_y - 1]) {
                    x = -grid_span_x/2 + i * pitch;
                    y = -grid_span_y/2 + j * pitch;
                    // Place sphere so bottom face hole = hole_d
                    translate([x, y, thickness/2 - offset])
                        sphere(d = marble_d, $fn = $fn_holes);
                }
        }

        // Standoffs
        for (i = [0 : num_x])
            for (j = [0 : num_y]) {
                x = -grid_span_x/2 + (i - 0.5) * pitch;
                y = -grid_span_y/2 + (j - 0.5) * pitch;
                translate([x, y, -standoff])
                    cylinder(h = standoff, d = standoff_d, $fn = $fn_holes);
            }
    }
}

// --- Call ---
hole_plate(num_x, num_y, marble_d, hole_d, pitch, margin, thickness, standoff, standoff_d);
