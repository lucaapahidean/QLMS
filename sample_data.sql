-- Connect to the database
\c qlms;

-- Clear existing data
TRUNCATE TABLE answers, quiz_attempts, question_options, questions, quizzes, text_lessons, course_materials, class_members, courses, classes, users RESTART IDENTITY CASCADE;

-- Insert sample users
-- All passwords are: 'password123'
INSERT INTO users (user_id, username, password_hash, role) VALUES 
(1, 'admin', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'admin'),
(2, 'instructor1', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'instructor'),
(3, 'instructor2', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'instructor'),
(4, 'student1', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student'),
(5, 'student2', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student'),
(6, 'student3', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student');

-- Insert classes
INSERT INTO classes (class_id, class_name) VALUES
(1, 'Fall 2025 Computer Science'),
(2, 'Spring 2026 Mathematics');

-- Insert courses
INSERT INTO courses (course_id, course_name, class_id) VALUES
(1, 'Intro to Programming', 1),
(2, 'Advanced Algorithms', 1),
(3, 'Calculus I', 2);

-- Assign users to classes
INSERT INTO class_members (class_id, user_id) VALUES
(1, 2), (1, 4), (1, 5), -- instructor1, student1, student2 in CS
(2, 3), (2, 6); -- instructor2, student3 in Math

-- Insert sample course materials
INSERT INTO course_materials (material_id, title, type, course_id, creator_id) VALUES 
(1, 'Introduction to Programming', 'lesson', 1, 2),
(2, 'Variables and Data Types', 'lesson', 1, 2),
(3, 'Control Structures', 'lesson', 1, 2),
(4, 'Big O Notation', 'lesson', 2, 2),
(5, 'Limits and Derivatives', 'lesson', 3, 3),
(6, 'Programming Basics Quiz', 'quiz', 1, 2),
(7, 'Advanced Sorting Quiz', 'quiz', 2, 2),
(8, 'Derivatives Quiz', 'quiz', 3, 3);

-- Insert lesson content
INSERT INTO text_lessons (lesson_id, content) VALUES 
(1, 'Welcome to Programming! This lesson covers the basics...'),
(2, 'Variables are containers for storing data values...'),
(3, 'Control structures determine the flow of execution...'),
(4, 'Big O notation is a mathematical notation that describes the limiting behavior...'),
(5, 'A limit is the value that a function approaches...');

-- Insert quiz details
INSERT INTO quizzes (quiz_id, max_attempts, feedback_type) VALUES 
(6, 3, 'detailed_with_answers'),
(7, 2, 'score_only'),
(8, 2, 'detailed_without_answers');

-- Insert questions
INSERT INTO questions (question_id, quiz_id, prompt, question_type) VALUES 
(1, 6, 'What is a variable in programming?', 'radio'),
(2, 6, 'Which of the following are primitive data types?', 'checkbox'),
(3, 6, 'Explain the difference between a compiler and an interpreter.', 'open_answer'),
(4, 7, 'What is the time complexity of Merge Sort?', 'radio'),
(5, 8, 'What is the derivative of x^2?', 'open_answer');

-- Insert question options
INSERT INTO question_options (option_id, question_id, option_text, is_correct) VALUES 
(1, 1, 'A fixed value', FALSE), 
(2, 1, 'A container for data', TRUE),
(3, 2, 'Integer', TRUE), 
(4, 2, 'String', TRUE), 
(5, 2, 'Array', FALSE),
(6, 4, 'O(n^2)', FALSE), 
(7, 4, 'O(n log n)', TRUE);

-- Insert sample quiz attempts
INSERT INTO quiz_attempts (attempt_id, quiz_id, student_id, attempt_number, status, auto_score, total_auto_points, total_manual_points, submitted_at) VALUES 
(1, 6, 4, 1, 'pending_manual_grading', 50.0, 2, 1, NOW() - INTERVAL '1 day'),
(2, 8, 6, 1, 'pending_manual_grading', 0, 0, 1, NOW() - INTERVAL '2 hours');

-- Insert answers for attempts
INSERT INTO answers (answer_id, attempt_id, question_id, student_response, is_correct, points_earned, max_points) VALUES 
(1, 1, 1, '1', TRUE, 1.0, 1.0),
(2, 1, 2, '0', FALSE, 0.0, 1.0),
(3, 1, 3, 'A compiler translates everything at once, an interpreter does it line by line.', NULL, NULL, 1.0),
(4, 2, 5, '2x', NULL, NULL, 1.0);

-- Fix sequences after explicit inserts
SELECT setval('users_user_id_seq', (SELECT MAX(user_id) FROM users));
SELECT setval('classes_class_id_seq', (SELECT MAX(class_id) FROM classes));
SELECT setval('courses_course_id_seq', (SELECT MAX(course_id) FROM courses));
SELECT setval('course_materials_material_id_seq', (SELECT MAX(material_id) FROM course_materials));
SELECT setval('questions_question_id_seq', (SELECT MAX(question_id) FROM questions));
SELECT setval('question_options_option_id_seq', (SELECT MAX(option_id) FROM question_options));
SELECT setval('quiz_attempts_attempt_id_seq', (SELECT MAX(attempt_id) FROM quiz_attempts));
SELECT setval('answers_answer_id_seq', (SELECT MAX(answer_id) FROM answers));

-- Display summary
SELECT 'Sample data loaded successfully!' as message;