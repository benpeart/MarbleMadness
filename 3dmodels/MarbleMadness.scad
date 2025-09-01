// =========================
// Marble Madness
// =========================

/* [Plate parameters] */
num_x       = 4;
num_y       = 4;
marble_d    = 16.18;     // mm diameter of largest marble (sphere)
hole_d      = 15.75;     // mm slightly smaller than the diameter of the smallest marble
pitch       = 16.67;     // mm (center-to-center)
margin      = 5;         // mm edge margin beyond outer hole edges
plate_height = 3;        // mm
led_height  = 2.15;      // mm
standoff_height = led_height + marble_d / 2; // height of standoffs
standoff_d  = 3;         // diameter of standoffs
notch_width = 11;        // 10mm for the strip and 1mm slop
notch_height = 1;


/* [Shadow Box parameters] */
wall_thickness = 3;
lip_width = 4;
lip_height = 3;

/* [Quality settings] */
$fn_fast    = 64;        // low-poly for speed
$fn_final   = 96;        // smooth for export

// --- Auto quality based on preview mode ---
$fn_holes   = $preview ? $fn_fast : $fn_final;

module hole_plate(num_x, num_y, marble_d, hole_d, pitch, margin, plate_height, standoff_height, standoff_d) {
    // Calculate plate dimensions
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
            cube([plate_x, plate_y, plate_height], center = true);

            // Holes
            for (i = [0 : num_x - 1])
                for (j = [0 : num_y - 1]) {
                    x = -grid_span_x/2 + i * pitch;
                    y = -grid_span_y/2 + j * pitch;
                    // Place sphere so bottom face hole = hole_d
                    translate([x, y, plate_height/2 - offset])
                        sphere(d = marble_d, $fn = $fn_holes);
                }
        }

        difference() {
            // Louvers to prevent light bleeding between cells
            for (i = [0 : num_x - 1])
                for (j = [0 : num_y - 1]) {
                    x = -grid_span_x/2 + i * pitch;
                    y = -grid_span_y/2 + j * pitch;
                    translate([x, y, -standoff_height - plate_height/2])
                        difference() {
                            cylinder(h = standoff_height, d = marble_d + wall_thickness/3, $fn = $fn_holes);
                            cylinder(h = standoff_height + 0.01, d = marble_d, $fn = $fn_holes);
                        }
                }

            // remove notches for the led strips
            for (i = [0 : num_x - 1])
                for (j = [0 : num_y - 1]) {
                    x = -grid_span_x/2 + i * pitch;
                    y = -grid_span_y/2 + j * pitch;
                    translate([x, y, -standoff_height - plate_height/2 + notch_height/2])
                        cube([plate_x, notch_width, notch_height + 0.01], center = true);
            }
        }
/*
        // Standoffs
        for (i = [0 : num_x])
            for (j = [0 : num_y]) {
                x = -grid_span_x/2 + (i - 0.5) * pitch;
                y = -grid_span_y/2 + (j - 0.5) * pitch;
                translate([x, y, -standoff_height - plate_height/2])
                    cylinder(h = standoff_height, d = standoff_d, $fn = $fn_holes);
            }
*/        
    }
}


// =========================
// Shadow Box Module
// =========================

module shadow_box(inner_x, inner_y, height, wall_thickness, lip_width, lip_height) {
    outer_x = inner_x + 2 * wall_thickness;
    outer_y = inner_y + 2 * wall_thickness;

    difference() {
        // Outer box
        cube([outer_x, outer_y, height], center = true);

        // Inner cavity
        translate([0, 0, -lip_height])
            cube([inner_x, inner_y, height], center = true);
        
        // front lip
        translate([0, 0, height/2 - lip_height/2])
            cube([inner_x - 2 * lip_width, inner_y - 2 * lip_width, lip_height + 0.01], center = true);
    }
}

// =========================
// Modified Call with Shadow Box
// =========================

render_shadow_box = false;
render_marble_plate = true;
render_back_plate = false;

module hole_plate_with_box() {
    // Calculate plate dimensions
    grid_span_x = (num_x - 1) * pitch;
    grid_span_y = (num_y - 1) * pitch;
    plate_x = grid_span_x + hole_d + 2 * margin;
    plate_y = grid_span_y + hole_d + 2 * margin;

    if (render_shadow_box)
        difference() {
            // Shadow box around the marble plate
            shadow_box_height = lip_height + plate_height + standoff_height + plate_height + plate_height / 2;
            translate([0, 0, lip_height -shadow_box_height/2 + plate_height/2])
                shadow_box(plate_x, plate_y, shadow_box_height, wall_thickness, lip_width, lip_height);
               
            // remove a notch for the LED power/data lines
            notch_width = 2;
            notch_height = plate_height * 2.5;
            outer_x = plate_x + 2 * wall_thickness;
            grid_span_y = (num_y - 1) * pitch;
            x = -outer_x/2 + wall_thickness / 2 - 0.01;
            y = grid_span_y/2 + 0 * pitch;
            z = -(standoff_height + notch_height/2 - plate_height / 2);
            translate([x - 0.01, y, z])
                cube([wall_thickness + 0.05, notch_width, notch_height + 0.01], center = true);
        }

    // Marble plate itself
    if (render_marble_plate)
        hole_plate(num_x, num_y, marble_d, hole_d, pitch, margin, plate_height, standoff_height, standoff_d);
    
    // Back plate
    if (render_back_plate)
        translate([0, 0, -standoff_height -plate_height])
            color("black") 
                cube([plate_x, plate_y, plate_height], center = true);
}

// --- Call ---
hole_plate_with_box();
