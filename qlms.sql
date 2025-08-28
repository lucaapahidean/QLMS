-- Drop and recreate database
DROP DATABASE IF EXISTS qlms;
CREATE DATABASE qlms;

-- Connect to qlms database
\c qlms;

-- Create tables
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) CHECK (role IN ('admin', 'instructor', 'student')) NOT NULL
);

CREATE TABLE course_materials (
    material_id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    type VARCHAR(20) CHECK (type IN ('lesson', 'quiz')) NOT NULL
);

CREATE TABLE text_lessons (
    lesson_id INTEGER PRIMARY KEY REFERENCES course_materials(material_id) ON DELETE CASCADE,
    content TEXT NOT NULL
);

CREATE TABLE quizzes (
    quiz_id INTEGER PRIMARY KEY REFERENCES course_materials(material_id) ON DELETE CASCADE,
    max_attempts INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE questions (
    question_id SERIAL PRIMARY KEY,
    quiz_id INTEGER NOT NULL REFERENCES quizzes(quiz_id) ON DELETE CASCADE,
    prompt TEXT NOT NULL,
    question_type VARCHAR(20) CHECK (question_type IN ('checkbox', 'radio', 'open_answer')) NOT NULL
);

CREATE TABLE question_options (
    option_id SERIAL PRIMARY KEY,
    question_id INTEGER NOT NULL REFERENCES questions(question_id) ON DELETE CASCADE,
    option_text TEXT NOT NULL,
    is_correct BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE TABLE quiz_attempts (
    attempt_id SERIAL PRIMARY KEY,
    quiz_id INTEGER NOT NULL REFERENCES quizzes(quiz_id) ON DELETE CASCADE,
    student_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    attempt_number INTEGER NOT NULL,
    status VARCHAR(30) CHECK (status IN ('completed', 'pending_manual_grading')) NOT NULL,
    final_score FLOAT,
    UNIQUE(quiz_id, student_id, attempt_number)
);

CREATE TABLE answers (
    answer_id SERIAL PRIMARY KEY,
    attempt_id INTEGER NOT NULL REFERENCES quiz_attempts(attempt_id) ON DELETE CASCADE,
    question_id INTEGER NOT NULL REFERENCES questions(question_id) ON DELETE CASCADE,
    student_response TEXT
);

-- Create indexes for better performance
CREATE INDEX idx_users_role ON users(role);
CREATE INDEX idx_course_materials_type ON course_materials(type);
CREATE INDEX idx_quiz_attempts_status ON quiz_attempts(status);
CREATE INDEX idx_quiz_attempts_student ON quiz_attempts(student_id);