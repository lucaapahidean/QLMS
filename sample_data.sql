-- Connect to the database
\c qlms;

-- Clear existing data
TRUNCATE TABLE answers CASCADE;
TRUNCATE TABLE quiz_attempts CASCADE;
TRUNCATE TABLE question_options CASCADE;
TRUNCATE TABLE questions CASCADE;
TRUNCATE TABLE quizzes CASCADE;
TRUNCATE TABLE text_lessons CASCADE;
TRUNCATE TABLE course_materials CASCADE;
TRUNCATE TABLE users CASCADE;

-- Insert sample users
-- All passwords are: 'password123'
INSERT INTO users (username, password_hash, role) VALUES 
('admin', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'admin'),
('instructor1', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'instructor'),
('instructor2', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'instructor'),
('student1', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student'),
('student2', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student'),
('student3', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'student');

-- Insert sample text lessons
INSERT INTO course_materials (material_id, title, type) VALUES 
(1, 'Introduction to Programming', 'lesson'),
(2, 'Variables and Data Types', 'lesson'),
(3, 'Control Structures', 'lesson');

INSERT INTO text_lessons (lesson_id, content) VALUES 
(1, 'Welcome to Programming! This lesson covers the basics of what programming is, why it''s important, and how computers execute code. Programming is the process of creating instructions that a computer can follow to perform specific tasks. In this course, we will explore fundamental concepts that form the foundation of all programming languages.'),
(2, 'Variables are containers for storing data values. In programming, we use different data types to represent different kinds of information. Common data types include integers (whole numbers), floating-point numbers (decimals), strings (text), and booleans (true/false values). Understanding data types is crucial for writing efficient and error-free code.'),
(3, 'Control structures determine the flow of execution in a program. The main control structures are: 1) Sequential execution (code runs line by line), 2) Selection (if-then-else statements), and 3) Iteration (loops like for and while). These structures allow us to create complex logic and solve real-world problems.');

-- Insert sample quizzes
INSERT INTO course_materials (material_id, title, type) VALUES 
(4, 'Programming Basics Quiz', 'quiz'),
(5, 'Data Types Assessment', 'quiz');

INSERT INTO quizzes (quiz_id, max_attempts) VALUES 
(4, 3),
(5, 2);

-- Insert questions for Quiz 1
INSERT INTO questions (question_id, quiz_id, prompt, question_type) VALUES 
(1, 4, 'What is a variable in programming?', 'radio'),
(2, 4, 'Which of the following are primitive data types? (Select all that apply)', 'checkbox'),
(3, 4, 'Explain the difference between a compiler and an interpreter.', 'open_answer');

-- Insert options for question 1
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(1, 'A fixed value that never changes', FALSE),
(1, 'A container for storing data values', TRUE),
(1, 'A type of loop', FALSE),
(1, 'A programming language', FALSE);

-- Insert options for question 2
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(2, 'Integer', TRUE),
(2, 'String', TRUE),
(2, 'Array', FALSE),
(2, 'Boolean', TRUE),
(2, 'Object', FALSE);

-- Insert questions for Quiz 2
INSERT INTO questions (question_id, quiz_id, prompt, question_type) VALUES 
(4, 5, 'What is the range of a 32-bit signed integer?', 'radio'),
(5, 5, 'Which statement about floating-point numbers is correct?', 'radio');

-- Insert options for question 4
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(4, '0 to 4,294,967,295', FALSE),
(4, '-2,147,483,648 to 2,147,483,647', TRUE),
(4, '-32,768 to 32,767', FALSE),
(4, '0 to 65,535', FALSE);

-- Insert options for question 5
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(5, 'They can represent all decimal numbers with perfect precision', FALSE),
(5, 'They may have rounding errors due to binary representation', TRUE),
(5, 'They use less memory than integers', FALSE),
(5, 'They can only represent positive numbers', FALSE);

-- Insert sample quiz attempts (some completed, some pending grading)
INSERT INTO quiz_attempts (quiz_id, student_id, attempt_number, status, final_score) VALUES 
(4, 4, 1, 'completed', 75.0),
(4, 4, 2, 'pending_manual_grading', NULL),
(4, 5, 1, 'pending_manual_grading', NULL);

-- Insert sample answers for pending attempts
INSERT INTO answers (attempt_id, question_id, student_response) VALUES 
(2, 1, '1'),  -- Selected option index 1
(2, 2, '0,1,3'),  -- Selected options 0, 1, and 3
(2, 3, 'A compiler translates the entire source code into machine code before execution, while an interpreter translates and executes code line by line.'),
(3, 1, '1'),
(3, 2, '0,1,2,3'),
(3, 3, 'Compilers are faster but interpreters are more flexible for development.');

-- Fix sequences after explicit inserts
SELECT setval('course_materials_material_id_seq', (SELECT MAX(material_id) FROM course_materials));
SELECT setval('questions_question_id_seq', (SELECT MAX(question_id) FROM questions));
SELECT setval('quiz_attempts_attempt_id_seq', (SELECT MAX(attempt_id) FROM quiz_attempts));
SELECT setval('users_user_id_seq', (SELECT MAX(user_id) FROM users));

-- Display summary
SELECT 'Sample data loaded successfully!' as message;
SELECT 'Users created:' as info, COUNT(*) as count FROM users;
SELECT 'Lessons created:' as info, COUNT(*) as count FROM text_lessons;
SELECT 'Quizzes created:' as info, COUNT(*) as count FROM quizzes;
SELECT 'Questions created:' as info, COUNT(*) as count FROM questions;
SELECT 'Pending quiz attempts:' as info, COUNT(*) as count FROM quiz_attempts WHERE status = 'pending_manual_grading';