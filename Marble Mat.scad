// =========================
// Hole Plate with Parametric Standoffs (Inset Perimeter)
// =========================

// --- Plate parameters ---
num_x       = 4;
num_y       = 4;
marble_d    = 16;        // mm diameter of marble
pitch       = 16.67;     // mm (center-to-center)
margin      = 10;        // mm edge margin beyond outer hole edges
thickness   = 3;         // mm
led_height  = 2.15;      // mm
standoff    = led_height + marble_d / 2; // height of standoffs
standoff_d  = 3;         // diameter of standoffs
//standoff_spacing = 25;   // mm spacing between perimeter standoffs

// --- Quality settings ---
$fn_fast    = 64;        // low-poly for speed
$fn_final   = 128;       // smooth for export

// --- Auto quality based on preview mode ---
$fn_holes   = $preview ? $fn_fast : $fn_final;

module hole_plate(nx, ny, hd, p, m, t) {
    grid_span_x = (nx - 1) * p;
    grid_span_y = (ny - 1) * p;

    plate_x = grid_span_x + hd + 2 * m;
    plate_y = grid_span_y + hd + 2 * m;

    inset = m / 2; // NEW: inset distance for perimeter standoffs

    union() {
        // Plate with holes
        difference() {
            cube([plate_x, plate_y, t], center = true);

            // Holes
            for (i = [0 : nx - 1])
                for (j = [0 : ny - 1]) {
                    x = -grid_span_x/2 + i * p;
                    y = -grid_span_y/2 + j * p;
                    translate([x, y])
                        sphere(d = hd, $fn = $fn_holes);
                }
        }

/*        
        // --- Perimeter standoffs evenly spaced, inset by margin/2 ---
        // Bottom & Top edges
        for (xpos = [-plate_x/2 + inset + standoff_spacing/2 : standoff_spacing : plate_x/2 - inset - standoff_spacing/2])
            for (ypos = [-plate_y/2 + inset, plate_y/2 - inset])
                translate([xpos, ypos, -standoff])
                    cylinder(h = standoff, d = standoff_d, $fn = $fn_holes);

        // Left & Right edges
        for (ypos = [-plate_y/2 + inset + standoff_spacing/2 : standoff_spacing : plate_y/2 - inset - standoff_spacing/2])
            for (xpos = [-plate_x/2 + inset, plate_x/2 - inset])
                translate([xpos, ypos, -standoff])
                    cylinder(h = standoff, d = standoff_d, $fn = $fn_holes);
*/
        // --- Between-hole standoffs ---
        for (i = [0 : nx])
            for (j = [0 : ny]) {
                x = -grid_span_x/2 + (i - 0.5) * p;
                y = -grid_span_y/2 + (j - 0.5) * p;
                translate([x, y, -standoff])
                    cylinder(h = standoff, d = standoff_d, $fn = $fn_holes);
            }
    }
}

// --- Call ---
hole_plate(num_x, num_y, marble_d, pitch, margin, thickness);
