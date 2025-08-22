// =========================
// Auto-Optimized Hole Plate
// =========================

// --- Plate parameters ---
num_x       = 4;
num_y       = 4;
marble_d    = 15.82;     // mm diameter of marble
pitch       = 16.67;     // mm (center-to-center)
margin      = 10;        // mm edge margin beyond outer hole edges
thickness   = 1;         // mm
led_height  = 2.15;      // mm
standoff    = led_height + marble_d / 2 - thickness / 2;

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

    difference() {
        linear_extrude(height = t)
            square([plate_x, plate_y], center = true);

        // Union all holes into one shape before subtraction
        difference() {
            for (i = [0 : nx - 1])
                for (j = [0 : ny - 1]) {
                    x = -grid_span_x/2 + i * p;
                    y = -grid_span_y/2 + j * p;
                    translate([x, y, thickness / 2]) sphere(d = hd, $fn = $fn_holes);
                }
        }
    }
}

// --- Call ---
hole_plate(num_x, num_y, marble_d, pitch, margin, thickness);
