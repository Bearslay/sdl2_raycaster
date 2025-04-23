#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "bengine.hpp"

class player_top_down {
    protected:
        bengine::basic_collider_2d collider = bengine::basic_collider_2d(0, 0, 0.5, 0.5);

        // \brief rotation of player (radians; 0 is to the right of the screen)
        double rotation = 0;
        // \brief speed of the player ([length-unit]s per [time-unit] | [m/s])
        double movespeed = 0.25;

    public:
        player_top_down() {}
        player_top_down(const double &x_pos, const double &y_pos, const double &rotation) {
            this->collider.set_x_pos(x_pos);
            this->collider.set_y_pos(y_pos);
            this->set_rotation(rotation);
        }

        double get_x_pos() const {
            return this->collider.get_x_pos();
        }
        double get_y_pos() const {
            return this->collider.get_y_pos();
        }
        double get_rotation() const {
            return this->rotation;
        }
        double get_movespeed() const {
            return this->movespeed;
        }
        double get_radius() const {
            return this->collider.get_width_2();
        }
        double get_diameter() const {
            return this->collider.get_width();
        }
        double get_left_x() const {
            return this->collider.get_left_x();
        }
        double get_right_x() const {
            return this->collider.get_right_x();
        }
        double get_top_y() const {
            return this->collider.get_top_y();
        }
        double get_bottom_y() const {
            return this->collider.get_bottom_y();
        }

        void set_x_pos(const double &x_pos) {
            this->collider.set_x_pos(x_pos);
        }
        void set_y_pos(const double &y_pos) {
            this->collider.set_y_pos(y_pos);
        }
        void set_rotation(const double &rotation) {
            this->rotation = bengine::math_helper::normalize_radian_angle(rotation);
        }
        void set_movespeed(const double &movespeed) {
            this->movespeed = movespeed < 0 ? 0 : movespeed;
        }
        void set_radius(const double &radius) {
            this->collider.set_width((radius <= 0 ? C_0_RIGHT : radius) * 2);
            this->collider.set_height(this->collider.get_width());
        }
        void set_diameter(const double &diameter) {
            this->collider.set_width(diameter <= 0 ? C_0_RIGHT * 2 : diameter);
            this->collider.set_height(this->collider.get_width());
        }

        void move_x(const double &amount) {
            this->set_x_pos(this->get_x_pos() + amount);
        }
        void move_y(const double &amount) {
            this->set_y_pos(this->get_y_pos() + amount);
        }

        void look_cw(const double &amount) {
            this->rotation = bengine::math_helper::normalize_radian_angle(this->rotation - amount);
        }
        void look_ccw(const double &amount) {
            this->rotation = bengine::math_helper::normalize_radian_angle(this->rotation + amount);
        }

        bool detect_collision(const bengine::basic_collider_2d &other) const {
            return this->collider.detect_collision(other);
        }

        bool fix_collision(bengine::basic_collider_2d &other, const bengine::basic_collider_2d::fix_mode &fix_mode, const bool &do_initial_collision_check = true) {
            return this->collider.fix_collision(other, fix_mode, do_initial_collision_check);
        }
};

class player_raycaster : public player_top_down {
    private:
        // \brief angle that represents the area that the player can see (radians)
        double fov = C_PI_2;
        // \brief how far one ray from the player will reach before expiring ([length-unit]s | [m])
        double view_distance = 5;
        // \brief how fast the player can look left/right (radians per second)
        double look_speed = C_PI_8;
        // \brief how fast the player can zoom in/out ([length-unit]s per second [m/s])
        double zoom_speed = 0.5;

    public:
        player_raycaster() : player_top_down() {}
        player_raycaster(const double &x_pos, const double &y_pos, const double &rotation) : player_top_down(x_pos, y_pos, rotation) {}
        player_raycaster(const double &x_pos, const double &y_pos, const double &rotation, const double &fov, const double &view_distance) : player_top_down(x_pos, y_pos, rotation) {
            this->set_fov(fov);
            this->set_view_distance(view_distance);
        }

        double get_fov() const {
            return this->fov;
        }
        double get_view_distance() const {
            return this->view_distance;
        }
        double get_look_speed() const {
            return this->look_speed;
        }
        double get_zoom_speed() const {
            return this->zoom_speed;
        }

        /** Set the FOV (field of view) of the player on the interval (0, 2*PI)
         * \param fov New FOV for the player
         */
        void set_fov(const double &fov) {
            this->fov = fov <= 0 ? C_0_RIGHT : (fov >= C_PI ? C_PI_LEFT : fov);
        }
        void set_view_distance(const double &view_distance) {
            this->view_distance = view_distance < 0 ? 0 : view_distance;
        }
        void set_look_speed(const double &look_speed) {
            this->look_speed = this->look_speed < 0 ? 0 : look_speed;
        }
        void set_zoom_speed(const double &zoom_speed) {
            this->zoom_speed = this->zoom_speed < 0 ? 0 : zoom_speed;
        }
};

class raycaster : public bengine::loop {
    private:
        struct {
            int quit = SDL_SCANCODE_ESCAPE;
            int move_forwards = SDL_SCANCODE_W;
            int move_backwards = SDL_SCANCODE_S;
            int strafe_left = SDL_SCANCODE_A;
            int strafe_right = SDL_SCANCODE_D;
            int look_left = SDL_SCANCODE_LEFT;
            int look_right = SDL_SCANCODE_RIGHT;
            int zoom_in = SDL_SCANCODE_UP;
            int zoom_out = SDL_SCANCODE_DOWN;
            int shrink_fov = SDL_SCANCODE_Q;
            int grow_fov = SDL_SCANCODE_E;
            int toggle_minimap = SDL_SCANCODE_M;
            int cycle_minimap_position = SDL_SCANCODE_P;
            int toggle_debug_screen = SDL_SCANCODE_F3;
        } keybinds;

        bengine::basic_texture minimap_texture;
        TTF_Font *font = TTF_OpenFont("dev/fonts/GNU-Unifont.ttf", 20);

        std::vector<std::vector<Uint8>> grid;

        /** 8-bit bitmask containing settings for the minimap
         * 
         * Formatting:
         * `Unused__ | Corner__ | Enabled_`
         * `00000___ | _____00_ | _______0`
         * 
         * Corner:
         * 0 = Top-Left, 1 = Top-Right, 2 = Bottom-Left, 3 = Bottom-Right
         * 
         * Enabled:
         * 0 = False, 1 = True
         */
        Uint8 minimap_settings = 3;
        Uint8 minimap_cell_size = 16;
        Uint16 minimap_side_length = 360;
        bool show_debug_screen = false;

        player_raycaster player;
        player_raycaster minimap_player = player_raycaster(this->minimap_side_length / 2, this->minimap_side_length / 2, this->player.get_rotation());
        bengine::hitscanner_2d hitscanner;

        std::vector<bengine::basic_collider_2d> colliders;

        double calc_move_angle(const bool &f, const bool &b, const bool &l, const bool &r) {
            if (f && !b) {
                if (l && !r) {
                    return C_3PI_4;
                } else if (!l && r) {
                    return C_PI_4;
                }
                return C_PI_2;
            } else if (!f && b) {
                if (l && !r) {
                    return C_5PI_4;
                } else if (!l && r) {
                    return C_7PI_4;
                }
                return C_3PI_2;
            }
            if (l && !r) {
                return C_PI;
            } else if (!l && r) {
                return 0;
            }
            return -1;
        }

        void handle_event() override {
            switch (this->event.type) {
                case SDL_KEYDOWN:
                    if (!this->event.key.repeat) {
                        if (this->keystate[this->keybinds.toggle_debug_screen]) {
                            this->show_debug_screen = !this->show_debug_screen;
                            this->visuals_changed = true;
                        }
                        if (this->keystate[this->keybinds.toggle_minimap]) {
                            if (bengine::bitwise_manipulator::get_bit_state<Uint8>(this->minimap_settings, 0)) {
                                this->minimap_settings = bengine::bitwise_manipulator::deactivate_bits<Uint8>(this->minimap_settings, 1);
                            } else {
                                this->minimap_settings = bengine::bitwise_manipulator::activate_bits<Uint8>(this->minimap_settings, 1);
                            }
                            this->visuals_changed = true;
                        }
                        if (this->keystate[this->keybinds.cycle_minimap_position]) {
                            this->minimap_settings = bengine::bitwise_manipulator::set_subvalue<Uint8>(this->minimap_settings, (bengine::bitwise_manipulator::get_subvalue<Uint8>(this->minimap_settings, 1, 2) + 1) % 4, 1, 2);
                            if (bengine::bitwise_manipulator::get_bit_state<Uint8>(this->minimap_settings, 0)) {
                                this->visuals_changed = true;
                            }
                        }
                    }
                    break;
            }
        }
        void compute() override {
            if (this->keystate[this->keybinds.quit]) {
                this->loop_running = false;
            }

            if (this->calc_move_angle(this->keystate[this->keybinds.move_forwards], this->keystate[this->keybinds.move_backwards], this->keystate[this->keybinds.strafe_left], this->keystate[this->keybinds.strafe_right]) >= 0) {
                const double move_angle = this->calc_move_angle(this->keystate[this->keybinds.move_forwards], this->keystate[this->keybinds.move_backwards], this->keystate[this->keybinds.strafe_left], this->keystate[this->keybinds.strafe_right]) - this->player.get_rotation() - C_PI_2;
                this->player.move_x(this->player.get_movespeed() * std::cos(move_angle) * this->delta_time);
                this->player.move_y(-this->player.get_movespeed() * std::sin(move_angle) * this->delta_time);
                this->hitscanner.set_x_pos(this->player.get_x_pos());
                this->hitscanner.set_y_pos(this->player.get_y_pos());
                this->visuals_changed = true;
            }

            if (this->keystate[this->keybinds.look_left]) {
                player.look_cw(this->player.get_look_speed() * this->delta_time);
                this->hitscanner.set_angle(this->hitscanner.get_angle() - this->player.get_look_speed() * this->delta_time);
                this->visuals_changed = true;
            } else if (this->keystate[this->keybinds.look_right]) {
                player.look_ccw(this->player.get_look_speed() * this->delta_time);
                this->hitscanner.set_angle(this->hitscanner.get_angle() + this->player.get_look_speed() * this->delta_time);
                this->visuals_changed = true;
            }
            if (this->keystate[this->keybinds.zoom_in]) {
                this->player.set_view_distance(this->player.get_view_distance() - this->player.get_zoom_speed() * this->delta_time);
                if (this->player.get_view_distance() < 1) {
                    this->player.set_view_distance(1);
                }
                this->hitscanner.set_range(this->player.get_view_distance());
                this->visuals_changed = true;
            } else if (this->keystate[this->keybinds.zoom_out]) {
                this->player.set_view_distance(this->player.get_view_distance() + this->player.get_zoom_speed() * this->delta_time);
                this->hitscanner.set_range(this->player.get_view_distance());
                this->visuals_changed = true;
            }
            if (this->keystate[this->keybinds.shrink_fov]) {
                this->player.set_fov(this->player.get_fov() - this->player.get_zoom_speed() * this->delta_time);
                this->visuals_changed = true;
            } else if (this->keystate[this->keybinds.grow_fov]) {
                this->player.set_fov(this->player.get_fov() + this->player.get_zoom_speed() * this->delta_time);
                this->visuals_changed = true;
            }

            for (std::size_t i = 0; i < this->colliders.size(); i++) {
                if (this->player.fix_collision(this->colliders[i], bengine::basic_collider_2d::fix_mode::MOVE_SELF, true)) {
                    this->hitscanner.set_x_pos(this->player.get_x_pos());
                    this->hitscanner.set_y_pos(this->player.get_y_pos());
                    this->visuals_changed = true;
                }
            }
        }

        void create_minimap_texture() {
            this->window.target_renderer_at_dummy();
            this->window.initialize_dummy(this->grid.at(0).size() * minimap_cell_size, this->grid.size() * minimap_cell_size);
            this->window.clear_renderer();

            for (std::size_t row = 0; row < this->grid.size(); row++) {
                for (std::size_t col = 0; col < this->grid.at(0).size(); col++) {
                    if (grid.at(row).at(col) > 0) {
                        this->window.fill_rectangle(col * minimap_cell_size, row * minimap_cell_size, minimap_cell_size, minimap_cell_size, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::WHITE));
                    }
                }
            }
            this->window.present_renderer();

            this->minimap_texture.set_texture(this->window.duplicate_dummy());
            this->window.target_renderer_at_window();
            this->window.clear_renderer();
        }
        void render() override {
            std::vector<std::optional<bengine::coordinate_2d<double>>> raycast_collisions;
            const double original_hitscanner_angle = this->hitscanner.get_angle();
            for (double angle = -this->player.get_fov() / 2; angle <= this->player.get_fov() / 2; angle += this->player.get_fov() / this->window.get_width()) {
                this->hitscanner.set_angle(original_hitscanner_angle + angle);
                raycast_collisions.emplace_back(this->hitscanner.get_hit(this->colliders));
                if (!raycast_collisions.back().has_value()) {
                    continue;
                }
                const bengine::fast_vector_2d<double> projection(std::fabs(player.get_x_pos() - raycast_collisions.back().value().get_x_pos()), std::fabs(player.get_y_pos() - raycast_collisions.back().value().get_y_pos()));
                const double distance = projection.get_magnitude() * std::cos(angle);

                const unsigned char rectangle_brightness = bengine::math_helper::map_value_to_range<double, unsigned char>(distance, 0, player.get_view_distance(), 255, 0);
                const int rectangle_height = bengine::math_helper::map_value_to_range<double, int>(distance, 0, player.get_view_distance(), this->window.get_height(), 0);
                this->window.fill_rectangle(raycast_collisions.size(), this->window.get_height_2() - rectangle_height / 2, 1, rectangle_height, {rectangle_brightness, rectangle_brightness, rectangle_brightness, 255});
            }
            this->hitscanner.set_angle(original_hitscanner_angle);

            // Minimap rendering
            if (bengine::bitwise_manipulator::get_bit_state<Uint8>(this->minimap_settings, 0)) {
                const Uint16 minimap_corner_offset = 32;
                const Uint16 minimap_x_pos = bengine::bitwise_manipulator::get_subvalue<Uint8>(this->minimap_settings, 1, 2) % 2 == 0 ? minimap_corner_offset : this->window.get_width() - this->minimap_side_length - minimap_corner_offset;
                const Uint16 minimap_y_pos = bengine::bitwise_manipulator::get_subvalue<Uint8>(this->minimap_settings, 1, 2) <= 1 ? minimap_corner_offset : this->window.get_height() - this->minimap_side_length - minimap_corner_offset;
                
                const double view_distance = this->player.get_view_distance() * 2 > this->grid.size() || this->player.get_view_distance() * 2 > this->grid.at(0).size() ? std::min(this->grid.size(), this->grid.at(0).size()) / 2 : this->player.get_view_distance();
                const Uint16 minimap_view_x_pos = this->player.get_x_pos() - view_distance < 0 ? 0 : (this->player.get_x_pos() + view_distance > this->grid.at(0).size() ? (this->grid.at(0).size() - view_distance * 2) * this->minimap_cell_size : (this->player.get_x_pos() - view_distance) * this->minimap_cell_size);
                const Uint16 minimap_view_y_pos = this->player.get_y_pos() - view_distance < 0 ? 0 : (this->player.get_y_pos() + view_distance > this->grid.size() ? (this->grid.size() - view_distance * 2) * this->minimap_cell_size : (this->player.get_y_pos() - view_distance) * this->minimap_cell_size);
                const double minimap_scale_factor = this->minimap_side_length / (2 * view_distance * this->minimap_cell_size) * this->minimap_cell_size;

                if (this->player.get_x_pos() < view_distance) {
                    this->minimap_player.set_x_pos(this->player.get_x_pos() * minimap_scale_factor);
                } else if (this->player.get_x_pos() > this->grid.at(0).size() - view_distance) {
                    this->minimap_player.set_x_pos(this->minimap_side_length - (this->grid.at(0).size() - this->player.get_x_pos()) * minimap_scale_factor);
                }
                if (this->player.get_y_pos() < view_distance) {
                    this->minimap_player.set_y_pos(this->player.get_y_pos() * minimap_scale_factor);
                } else if (this->player.get_y_pos() > this->grid.size() - view_distance) {
                    this->minimap_player.set_y_pos(this->minimap_side_length - (this->grid.size() - this->player.get_y_pos()) * minimap_scale_factor);
                }

                this->window.fill_rectangle(minimap_x_pos - this->minimap_side_length / 30, minimap_y_pos - this->minimap_side_length / 30, this->minimap_side_length + this->minimap_side_length / 15, this->minimap_side_length + this->minimap_side_length / 15, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::DARK_GRAY));
                this->minimap_texture.set_frame({minimap_view_x_pos, minimap_view_y_pos, (int)(view_distance * this->minimap_cell_size * 2), (int)(view_distance * this->minimap_cell_size * 2)});
                this->window.render_basic_texture(this->minimap_texture, {minimap_x_pos, minimap_y_pos, this->minimap_side_length, this->minimap_side_length});

                for (std::size_t i = 0; i < raycast_collisions.size(); i++) {
                    if (raycast_collisions.at(i).has_value()) {
                        if (minimap_player.get_x_pos() + (raycast_collisions.at(i).value().get_x_pos() - this->player.get_x_pos()) * minimap_scale_factor < 0 || minimap_player.get_x_pos() + (raycast_collisions.at(i).value().get_x_pos() - this->player.get_x_pos()) * minimap_scale_factor > this->minimap_side_length || minimap_player.get_y_pos() + (raycast_collisions.at(i).value().get_y_pos() - this->player.get_y_pos()) * minimap_scale_factor < 0 || minimap_player.get_y_pos() + (raycast_collisions.at(i).value().get_y_pos() - this->player.get_y_pos()) * minimap_scale_factor > this->minimap_side_length) {
                            const double angle = this->hitscanner.get_angle() - this->player.get_fov() / 2 + i * this->player.get_fov() / this->window.get_width();
                            this->window.draw_line(minimap_x_pos + minimap_player.get_x_pos(), minimap_y_pos + minimap_player.get_y_pos(), minimap_x_pos + minimap_player.get_x_pos() + view_distance * std::cos(angle) * minimap_scale_factor, minimap_y_pos + minimap_player.get_y_pos() + view_distance * std::sin(angle) * minimap_scale_factor, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::LIGHT_GRAY));
                        } else {
                            this->window.draw_line(minimap_x_pos + minimap_player.get_x_pos(), minimap_y_pos + minimap_player.get_y_pos(), minimap_x_pos + minimap_player.get_x_pos() + (raycast_collisions.at(i).value().get_x_pos() - this->player.get_x_pos()) * minimap_scale_factor, minimap_y_pos + minimap_player.get_y_pos() + (raycast_collisions.at(i).value().get_y_pos() - this->player.get_y_pos()) * minimap_scale_factor, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::LIGHT_GRAY));
                        }
                    } else {
                        if (this->hitscanner.get_range() >= 0) {
                            const double angle = this->hitscanner.get_angle() - this->player.get_fov() / 2 + i * this->player.get_fov() / this->window.get_width();
                            this->window.draw_line(minimap_x_pos + minimap_player.get_x_pos(), minimap_y_pos + minimap_player.get_y_pos(), minimap_x_pos + minimap_player.get_x_pos() + view_distance * std::cos(angle) * minimap_scale_factor, minimap_y_pos + minimap_player.get_y_pos() + view_distance * std::sin(angle) * minimap_scale_factor, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::DARK_GRAY));
                        }
                    }
                }

                minimap_player.set_radius(this->player.get_radius() * (this->minimap_side_length / (2 * view_distance * this->minimap_cell_size)) * this->minimap_cell_size);
                this->window.fill_rectangle(minimap_x_pos + minimap_player.get_x_pos() - minimap_player.get_radius(), minimap_y_pos + minimap_player.get_y_pos() - minimap_player.get_radius(), minimap_player.get_radius() * 2, minimap_player.get_radius() * 2, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::RED));
            }

            // Debug screen rendering
            if (this->show_debug_screen) {
                this->window.fill_rectangle(0, 0, 310, 25, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::BLACK));
                this->window.render_text(this->font, bengine::string_helper::to_u16string("(" + bengine::string_helper::to_string_with_added_zeros<double>(this->player.get_x_pos(), 2, 5) + ", " + bengine::string_helper::to_string_with_added_zeros<double>(this->player.get_y_pos(), 2, 5) + ", " + bengine::string_helper::to_string_with_added_zeros<double>(this->hitscanner.get_angle() * U_180_PI, 3, 5) + ")").c_str(), 0, 0);
                this->window.render_SDLTexture(this->minimap_texture.get_texture(), {0, 0, (int)(this->grid.at(0).size() * this->minimap_cell_size), (int)(this->grid.size() * this->minimap_cell_size)}, {50, 50, (int)(this->grid.at(0).size() * this->minimap_cell_size), (int)(this->grid.size() * this->minimap_cell_size)});
            
                for (std::size_t i = 0; i < this->colliders.size(); i++) {
                    this->window.draw_rectangle(51 + this->colliders.at(i).get_left_x() * this->minimap_cell_size, 51 + this->colliders.at(i).get_bottom_y() * this->minimap_cell_size, this->colliders.at(i).get_width() * this->minimap_cell_size - 2, this->colliders.at(i).get_height() * this->minimap_cell_size - 2, {255, 0, 0, 255});
                }

                for (std::size_t i = 0; i < raycast_collisions.size(); i++) {
                    if (raycast_collisions.at(i).has_value()) {
                        this->window.draw_line(50 + this->hitscanner.get_x_pos() * this->minimap_cell_size, 50 + this->hitscanner.get_y_pos() * this->minimap_cell_size, 50 + raycast_collisions.at(i).value().get_x_pos() * this->minimap_cell_size, 50 + raycast_collisions.at(i).value().get_y_pos() * this->minimap_cell_size, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::LIME));
                    } else {
                        if (this->hitscanner.get_range() >= 0) {
                            const double angle = this->hitscanner.get_angle() - this->player.get_fov() / 2 + i * this->player.get_fov() / this->window.get_width();
                            this->window.draw_line(50 + this->hitscanner.get_x_pos() * this->minimap_cell_size, 50 + this->hitscanner.get_y_pos() * this->minimap_cell_size, 50 + this->hitscanner.get_x_pos() * this->minimap_cell_size + this->hitscanner.get_range() * std::cos(angle) * this->minimap_cell_size, 50 + this->hitscanner.get_y_pos() * this->minimap_cell_size + this->hitscanner.get_range() * std::sin(angle) * this->minimap_cell_size, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::GREEN));
                        }
                    }
                }

                this->window.fill_rectangle(50 + (this->player.get_x_pos() - this->player.get_radius()) * this->minimap_cell_size, 50 + (this->player.get_y_pos() - this->player.get_radius()) * this->minimap_cell_size, this->player.get_radius() * this->minimap_cell_size * 2, this->player.get_radius() * this->minimap_cell_size * 2, bengine::render_window::get_color_from_preset(bengine::render_window::preset_color::RED));
            }
        }

    public:
        raycaster(const std::vector<std::vector<Uint8>> &grid) : bengine::loop("raycaster", 1280, 720, SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/) {
            // in the case of an empty input grid, a 16x16 box is created as a "default"
            if (grid.empty()) {
                this->grid = {
                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
                    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
                };
                this->colliders.emplace_back(bengine::basic_collider_2d(8, 0.5, 16, 1));
                this->colliders.emplace_back(bengine::basic_collider_2d(0.5, 8.5, 1, 15));
                this->colliders.emplace_back(bengine::basic_collider_2d(15.5, 8.5, 1, 15));
                this->colliders.emplace_back(bengine::basic_collider_2d(8, 15.5, 14, 1));
            } else {
                // Basic copying of an input vector to and output, but also ensures that the output is rectangular
                std::size_t longest_row_cols = 0;
                for (std::size_t row = 0; row < grid.size(); row++) {
                    this->grid.emplace_back();
                    for (std::size_t col = 0; col < grid.at(row).size(); col++) {
                        this->grid[row].emplace_back(grid.at(row).at(col));
                    }
                    if (this->grid.at(row).size() > longest_row_cols) {
                        longest_row_cols = this->grid.at(row).size();
                    }
                }
                for (std::size_t row = 0; row < this->grid.size(); row++) {
                    for (std::size_t col = this->grid.at(row).size(); col < longest_row_cols; col++) {
                        this->grid[row].emplace_back(0);
                    }
                }
                
                // Algorithm to generate colliders with, making sure that any colliders that can be merged are merged

                // Create a grid that will hold whether a cell has been visited or not
                std::vector<std::vector<bool>> visit_grid(this->grid.size(), std::vector<bool>(this->grid.at(0).size(), false));
                for (std::size_t row = 0; row < this->grid.size(); row++) {
                    for (std::size_t col = 0; col < this->grid.at(0).size(); col++) {
                        if (this->grid.at(row).at(col) == 0) {
                            visit_grid[row][col] = true;
                        }
                    }
                }

                std::size_t row_start = 0, col_start = 0;
                while (row_start < this->grid.size() && col_start < this->grid.at(0).size()) {
                    // The new starting row/column is found by searching for the next spot that is unvisited
                    bool found_unvisited_cell = false;
                    while (row_start < this->grid.size()) {
                        while (col_start < this->grid.at(0).size()) {
                            if (!visit_grid.at(row_start).at(col_start)) {
                                found_unvisited_cell = true;
                                break;
                            }
                            col_start++;
                        }
                        if (found_unvisited_cell) {
                            break;
                        }
                        row_start++;
                        col_start = 0;
                        found_unvisited_cell = false;
                    }
                    if (!found_unvisited_cell) {
                        break;
                    }

                    std::size_t row_end = row_start, col_end = col_start;
                    // First, start by going to the right until reached a cell visited before (which either means that its blank or has been used already; it can't be included in either case)
                    while (col_end < this->grid.at(0).size()) {
                        if (visit_grid.at(row_end).at(col_end)) {
                            break;
                        }
                        visit_grid[row_end][col_end] = true;
                        col_end++;
                    }
                    // Now the mesh has a width, so now we go down with that width until a row has a cell that has been visited before
                    while (row_end < this->grid.size() - 1) {
                        row_end++;

                        // Check to see if the next row is allowed to be added to the mesh
                        bool valid_row = true;
                        for (std::size_t col = col_start; col < col_end; col++) {
                            if (visit_grid.at(row_end).at(col)) {
                                valid_row = false;
                                break;
                            }
                        }

                        // If the new row isn't valid, then the row_end is retracted back to a valid row and the start/end points of the mesh are defined
                        if (!valid_row) {
                            row_end--;
                            break;
                        }
                        // If the new row is valid, then the entire row is marked as visited as it will be consumed by the mesh
                        for (std::size_t i = col_start; i < col_end; i++) {
                            visit_grid[row_end][i] = true;
                        }
                    }

                    // At this point, there should be the top-left and bottom-right corners of a new mesh defined
                    // Here, a new collider is added based upon the start/end positions of the mesh
                    this->colliders.emplace_back(bengine::basic_collider_2d(col_start + static_cast<double>(col_end - col_start) / 2, row_start + static_cast<double>(row_end - row_start + 1) / 2, col_end - col_start, row_end - row_start + 1));
                }
            }

            this->create_minimap_texture();
            this->player.set_x_pos(this->grid.at(0).size() / 2);
            this->player.set_y_pos(this->grid.size() / 2);
            this->player.set_movespeed(0.25);
            this->hitscanner = bengine::hitscanner_2d(this->player.get_x_pos(), this->player.get_y_pos(), 0, this->player.get_view_distance(), false);
        }
        ~raycaster() {
            TTF_CloseFont(this->font);
        }
};

int main(int argc, char* args[]) {
    std::vector<std::vector<Uint8>> grid = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,1,1,0,0,0,1},
        {1,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,0,0,0,1},
        {1,0,0,1,1,0,0,1,1,1,0,0,1,0,1,0,0,0,0,0,0,0,0,1,1,1},
        {1,0,0,1,1,0,0,0,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0,0,0,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
        {1,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,1,0,0,1,0,0,0,1},
        {1,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,1,1,0,0,0,0,0,1},
        {1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,1,0,0,1,1},
        {1,0,1,1,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,1,0,0,0,0,1,1},
        {1,0,1,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1},
        {1,0,0,0,0,0,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,1,1,0,0,1},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
    };
    raycaster r(grid);
    return r.run();
}
