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

-- Insert sample quizzes with different feedback types
INSERT INTO course_materials (material_id, title, type) VALUES 
(4, 'Programming Basics Quiz (Detailed Feedback)', 'quiz'),
(5, 'Data Types Assessment (Score Only)', 'quiz'),
(6, 'Control Structures Quiz (Right/Wrong Only)', 'quiz');

INSERT INTO quizzes (quiz_id, max_attempts, feedback_type) VALUES 
(4, 3, 'detailed_with_answers'),  -- Shows correct answers
(5, 2, 'score_only'),              -- Shows only score
(6, 2, 'detailed_without_answers'); -- Shows right/wrong but not correct answers

-- Insert questions for Quiz 1 (Detailed Feedback)
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

-- Insert questions for Quiz 2 (Score Only)
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

-- Insert questions for Quiz 3 (Right/Wrong Only)
INSERT INTO questions (question_id, quiz_id, prompt, question_type) VALUES 
(6, 6, 'Which loop structure is best for iterating a known number of times?', 'radio'),
(7, 6, 'What are the components of a for loop? (Select all that apply)', 'checkbox'),
(8, 6, 'Describe a scenario where a while loop would be more appropriate than a for loop.', 'open_answer');

-- Insert options for question 6
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(6, 'while loop', FALSE),
(6, 'for loop', TRUE),
(6, 'do-while loop', FALSE),
(6, 'infinite loop', FALSE);

-- Insert options for question 7
INSERT INTO question_options (question_id, option_text, is_correct) VALUES 
(7, 'Initialization', TRUE),
(7, 'Condition', TRUE),
(7, 'Function call', FALSE),
(7, 'Increment/Update', TRUE),
(7, 'Return statement', FALSE);

-- Insert sample quiz attempts with auto-grading data
-- Completed attempt with auto-grading only (no open answers)
INSERT INTO quiz_attempts (attempt_id, quiz_id, student_id, attempt_number, status, auto_score, final_score, total_auto_points, total_manual_points, submitted_at, graded_at) VALUES 
(1, 5, 4, 1, 'completed', 100.0, 100.0, 2, 0, NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days');

-- Insert answers for attempt 1
INSERT INTO answers (attempt_id, question_id, student_response, is_correct, points_earned, max_points) VALUES 
(1, 4, '1', TRUE, 1.0, 1.0),   -- Correct answer
(1, 5, '1', TRUE, 1.0, 1.0);   -- Correct answer

-- Pending attempt with open answer questions
INSERT INTO quiz_attempts (attempt_id, quiz_id, student_id, attempt_number, status, auto_score, total_auto_points, total_manual_points, submitted_at) VALUES 
(2, 4, 4, 1, 'pending_manual_grading', 50.0, 2, 1, NOW() - INTERVAL '1 day');

-- Insert answers for attempt 2
INSERT INTO answers (attempt_id, question_id, student_response, is_correct, points_earned, max_points) VALUES 
(2, 1, '1', TRUE, 1.0, 1.0),  -- Correct answer
(2, 2, '0,2', FALSE, 0.0, 1.0),  -- Incorrect (missing String and Boolean)
(2, 3, 'A compiler translates the entire source code into machine code before execution, while an interpreter translates and executes code line by line.', NULL, NULL, 1.0); -- Pending manual grading

-- Another pending attempt
INSERT INTO quiz_attempts (attempt_id, quiz_id, student_id, attempt_number, status, auto_score, total_auto_points, total_manual_points, submitted_at) VALUES 
(3, 6, 5, 1, 'pending_manual_grading', 100.0, 2, 1, NOW() - INTERVAL '3 hours');

-- Insert answers for attempt 3
INSERT INTO answers (attempt_id, question_id, student_response, is_correct, points_earned, max_points) VALUES 
(3, 6, '1', TRUE, 1.0, 1.0),  -- Correct answer
(3, 7, '0,1,3', TRUE, 1.0, 1.0),  -- Correct answer
(3, 8, 'A while loop is better when you don''t know in advance how many iterations will be needed, such as reading input until a sentinel value is encountered.', NULL, NULL, 1.0); -- Pending manual grading

-- Fix sequences after explicit inserts
SELECT setval('course_materials_material_id_seq', (SELECT MAX(material_id) FROM course_materials));
SELECT setval('questions_question_id_seq', (SELECT MAX(question_id) FROM questions));
SELECT setval('quiz_attempts_attempt_id_seq', (SELECT MAX(attempt_id) FROM quiz_attempts));
SELECT setval('answers_answer_id_seq', (SELECT MAX(answer_id) FROM answers));
SELECT setval('users_user_id_seq', (SELECT MAX(user_id) FROM users));

-- Display summary
SELECT 'Sample data loaded successfully!' as message;
SELECT 'Users created:' as info, COUNT(*) as count FROM users;
SELECT 'Lessons created:' as info, COUNT(*) as count FROM text_lessons;
SELECT 'Quizzes created:' as info, COUNT(*) as count FROM quizzes;
SELECT '  - Detailed with answers:' as info, COUNT(*) as count FROM quizzes WHERE feedback_type = 'detailed_with_answers';
SELECT '  - Score only:' as info, COUNT(*) as count FROM quizzes WHERE feedback_type = 'score_only';
SELECT '  - Right/wrong only:' as info, COUNT(*) as count FROM quizzes WHERE feedback_type = 'detailed_without_answers';
SELECT 'Questions created:' as info, COUNT(*) as count FROM questions;
SELECT 'Quiz attempts:' as info, COUNT(*) as count FROM quiz_attempts;
SELECT '  - Completed:' as info, COUNT(*) as count FROM quiz_attempts WHERE status = 'completed';
SELECT '  - Pending grading:' as info, COUNT(*) as count FROM quiz_attempts WHERE status = 'pending_manual_grading';